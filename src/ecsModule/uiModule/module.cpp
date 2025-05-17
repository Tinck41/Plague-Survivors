#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "raylib.h"
#include "ext/matrix_transform.hpp"
#include "gtc/quaternion.hpp"
#include "resourceManager.h"
#include "ecsModule/utils.h"

#include <algorithm>
#include <functional>

using namespace ps;

UiModule::UiModule(flecs::world& world) {
	world.module<UiModule>();

	world.import<TransformModule>();

	world.component<Anchor>();
	world.component<Pivot>();
	world.component<BackgroundColor>();
	world.component<Interaction>();
	world.component<UiRenderQueue>();
	world.component<Node>()
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<Anchor>())
		.add(flecs::With, world.component<Pivot>())
		.add(flecs::With, world.component<BackgroundColor>());
	world.component<RootNode>()
		.add(flecs::With, world.component<Node>())
		.add(flecs::Exclusive);
	world.component<Primitive>()
		.add(flecs::With, world.component<Node>());
	world.component<Image>()
		.add(flecs::With, world.component<Node>());
	world.component<Text>()
		.add(flecs::With, world.component<Node>());
	world.component<Button>()
		.add(flecs::With, world.component<Node>())
		.add(flecs::With, world.component<Interaction>());

	world.component<Node>()
		.member<glm::vec2>("pos")
		.member<glm::vec2>("size");

	world.component<Anchor>()
		.member<float>("x")
		.member<float>("y");

	world.component<Pivot>()
		.member<float>("x")
		.member<float>("y");

	world.component<BackgroundColor>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a");

	world.component<std::string>()
		.opaque(flecs::String)
			.serialize([](const flecs::serializer *s, const std::string *data) {
				const char *str = data->c_str();
				return s->value(flecs::String, &str);
			})
			.assign_string([](std::string* data, const char *value) {
				*data = value;
			});

	world.component<Text>()
		.member<std::string>("string")
		.member<float>("fontSize")
		.member<float>("spacing");

	world.component<Interaction::eType>()
		.constant("None", Interaction::eType::None)
		.constant("Hovered", Interaction::eType::Hovered)
		.constant("Clicked", Interaction::eType::Clicked);

	world.component<Interaction>()
		.member<Interaction::eType>("type");

	world.component<Primitive>()
		.member<glm::vec2>("size");

	world.component<Image>()
		.member<std::string>("path")
		.member<Texture2D>("texture")
		.member<std::optional<Rectangle>>("rect");

	world.component<Button>()
		.member<glm::vec2>("size")
		.member<Color>("defaultColor")
		.member<Color>("hoverColor")
		.member<Color>("clickColor");

	auto root = world.entity(UI_ROO_ID).add<RootNode>().add(flecs::OrderedChildren);

	world.observer<Image>()
		.event(flecs::OnSet)
		.each([](Image& i) {
			i.texture = ResourceManager::getInstance()->getTexture(i.path);
		});

	world.observer<Node>()
		.event(flecs::OnSet)
		.each([root](flecs::iter& it, size_t size, Node node) {
			if (root.has<Dirty>()) {
				return;
			}

			root.add<Dirty>();
		});

	world.system<Node, GlobalTransform, Primitive>()
		.with<Dirty>()
		.kind(Phases::Update)
		.each([](Node& n, GlobalTransform& t, Primitive& p) {
			n.size = p.size * glm::vec2{ t.scale };
		});

	world.system<Node, GlobalTransform, Image>()
		.with<Dirty>()
		.kind(Phases::Update)
		.each([](Node& n, GlobalTransform& t, Image& i) {
			n.size = glm::vec2{ i.texture->width, i.texture->height } * glm::vec2{ t.scale };
		});

	world.system<Node, GlobalTransform, Text>()
		.with<Dirty>()
		.kind(Phases::Update)
		.each([](Node& n, GlobalTransform& t, Text& text) {
			const auto fontRect = MeasureTextEx(GetFontDefault(), text.string.c_str(), text.fontSize * t.scale.x, text.spacing);
			n.size = glm::vec2{ fontRect.x, fontRect.y } * glm::vec2{ t.scale };
		});

	world.system<RootNode>()
		.with<Dirty>()
		.kind(Phases::Update) // mb post update;
		.each([](flecs::entity root, RootNode) {
			auto zIndex = -1;
			utils::dfs(root, [&zIndex](flecs::entity e) {
				if (e.has<Node>()) {
					e.get_ref<Transform>()->translation.z = ++zIndex;
				}
			});
		});

	world.system<Node*, Node, GlobalTransform, Anchor, Pivot>()
		.term_at(0).parent().cascade().cached()
		.kind(Phases::Update) // mb post update;
		.each([](Node* pNode, Node& cNode, GlobalTransform& cTransform, Anchor& anc, Pivot& piv) {
			auto parentSize = pNode ? glm::vec2{ pNode->size.x, pNode->size.y } : glm::vec2{ 600, 600 };

			cNode.matrix = glm::mat4{ 1.f };
			cNode.matrix = glm::translate(cNode.matrix, glm::vec3{ parentSize * anc, 0 });
			cNode.matrix = glm::translate(cNode.matrix, glm::vec3{ cNode.size * -piv, 0 });

			if (pNode) {
				cNode.matrix = pNode->matrix * cNode.matrix;
			}

			auto matrix = cTransform.matrix * cNode.matrix;

			cNode.pos = glm::vec2(matrix[3]);
		});

	world.system<Input, Interaction, const Node, const GlobalTransform>()
		.term_at(0).inout(flecs::InOut).singleton()
		.kind(Phases::Update)
		.order_by<GlobalTransform>([](flecs::entity_t e1, const GlobalTransform *d1, flecs::entity_t e2, const GlobalTransform *d2) {
			return (d1->translation.z < d2->translation.z) - (d1->translation.z > d2->translation.z);
		})
		.run([](flecs::iter& it) {
			auto hovered = false;

			while (it.next()) {
				auto input       = it.field<Input>(0)[0];
				auto interaction = it.field<Interaction>(1);
				auto node        = it.field<const Node>(2);

				for (auto i : it ) {
					if (CheckCollisionPointRec({ input.mouse.position.x, input.mouse.position.y }, { node[i].pos.x, node[i].pos.y, node[i].size.x, node[i].size.y })) {
						if (!hovered && interaction[i].type != Interaction::eType::Hovered) {
							interaction[i].type = Interaction::eType::Hovered;
							hovered = true;
						} else {
							if (hovered) {
								interaction[i].type = Interaction::eType::None;
							} else {
								hovered = true;
							}
						}

						if (hovered && input.mouse.left.remain) {
							interaction[i].type = Interaction::eType::Clicked;
						}
					} else {
						interaction[i].type = Interaction::eType::None;
					}
				}
			}
		});

	world.system<const Interaction, const Button, BackgroundColor>()
		.kind(Phases::Update)
		.each([](flecs::entity e, const Interaction& i, const Button& b, BackgroundColor& c) {
			switch (i.type) {
				case Interaction::eType::Hovered:
					c = b.hoverColor;
					break;
				case Interaction::eType::Clicked:
					c = b.clickColor;
					break;
				case Interaction::eType::None:
					c = b.defaultColor;
					break;
				default:
					break;
			}
		});

	world.system<UiRenderQueue, Node, GlobalTransform, BackgroundColor, Primitive>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](UiRenderQueue& queue, Node& n, GlobalTransform& t, BackgroundColor& c, Primitive& p) {
			queue.renderCommands.emplace_back(
				t.translation.z,
				PrimitiveRenderData{
					.rec      = { n.pos.x, n.pos.y, n.size.x, n.size.y },
					.rotation = t.rotation.x,
					.color    = c
				}
			);
		});

	world.system<UiRenderQueue, Node, GlobalTransform, BackgroundColor, Image>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](UiRenderQueue& queue, Node& n, GlobalTransform& t, BackgroundColor& c, Image& i) {
			const auto source = [i]() {
				if (i.part) {
					return i.part.value();
				}

				return Rectangle{ 0.f, 0.f, static_cast<float>(i.texture->width), static_cast<float>(i.texture->height) };
			}();

			queue.renderCommands.emplace_back(
				t.translation.z,
				ImageRenderData{
					.texture  = *i.texture,
					.source   = source,
					.dest     = { n.pos.x, n.pos.y, n.size.x, n.size.y },
					.rotation = t.rotation.x,
					.color    = c,
				}
			);
		});

	world.system<UiRenderQueue, Node, GlobalTransform, BackgroundColor, Text>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](UiRenderQueue& queue, Node& n, GlobalTransform& t, BackgroundColor& c, Text& text) {
			queue.renderCommands.emplace_back(
				t.translation.z,
				TextRenderData{
					.font     = GetFontDefault(),
					.text     = text.string.c_str(),
					.position = { n.pos.x, n.pos.y },
					.rotation = t.rotation.x,
					.fontSize = text.fontSize * t.scale.x,
					.spacing  = text.spacing,
					.color    = c
				}
			);
		});


	world.system<UiRenderQueue>("NodeRenderSystem")
		.term_at(0).singleton()
		.kind(Phases::RenderUI)
		.each([](UiRenderQueue& queue) {
			std::ranges::sort(queue.renderCommands, [](const UiRenderCommand& lhs, const UiRenderCommand& rhs) {
				return lhs.sortIndex < rhs.sortIndex;
			});

			for (const auto& command : queue.renderCommands) {
				std::visit(UiRenderCommandDispatcher{}, command.renderData);
			}

			queue.renderCommands.clear();
		});

	world.add<UiRenderQueue>();
	root.set<Node>({
		.pos{ 0, 0 },
		.size{ 600, 600 }
	});
}

