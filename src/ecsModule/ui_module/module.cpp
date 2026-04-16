#include "module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/utils.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "font.h"
#include "layout.h"

#include <algorithm>
#include <vector>

using namespace ps;

UiModule::UiModule(flecs::world& world) {
	world.module<UiModule>();

	world.import<TransformModule>();
	world.import<InputModule>();
	world.import<TextModule>();
	world.import<CameraModule>();

	world.component<LayoutComposer>()
		.add(flecs::Singleton);

	world.component<NodeVector>()
		.add(flecs::Singleton);

	world.component<UiTreeChanged>()
		.add(flecs::Singleton);

	world.component<CustomNodeIndex>()
		.member<size_t>("index");

	world.component<BackgroundColor>()
		.is_a<Color>();

	world.component<Node::Grow>()
		.member("min", &Node::Grow::min)
		.member("max", &Node::Grow::max);

	world.component<Node::Fit>()
		.member("min", &Node::Grow::min)
		.member("max", &Node::Grow::max);

	world.component<Node::Fixed>()
		.member("value", &Node::Fixed::value);

	world.component<Node::GrowDirection>()
		.constant("Vertical", Node::GrowDirection::Vertical)
		.constant("Horizontal", Node::GrowDirection::Horizontal);

	world.component<Node>()
		.member("sizing_policy", &Node::sizing_policy)
		.member("self_alignment", &Node::self_alignment)
		.member("test", &Node::test)
		.member("size", &Node::size)
		.member("child_alignment", &Node::child_alignment)
		.member("stack_index", &Node::stack_index)
		.member("child_gap", &Node::child_gap)
		.member("margin", &Node::margin)
		.member("padding", &Node::padding)
		.member("grow_direction", &Node::grow_direction)
		.add(flecs::With, world.component<RenderLayers>())
		.add(flecs::With, world.component<Aabb>())
		.add(flecs::With, world.component<Visible2d>())
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<BackgroundColor>())
		.add(flecs::With, flecs::OrderedChildren);

	world.component<Image>()
		.add(flecs::With, world.component<Node>());

	world.component<Text>()
		.is_a<std::string>()
		.add(flecs::With, world.component<TextData>())
		.add(flecs::With, world.component<TextFont>())
		.add(flecs::With, world.component<TextColor>())
		.add(flecs::With, world.component<Node>());

	world.component<Interaction>()
		.constant("None", Interaction::None)
		.constant("Hovered", Interaction::Hovered)
		.constant("Clicked", Interaction::Clicked)
		.add(flecs::Exclusive);

	world.component<FocusStrategy>()
		.constant("Block", FocusStrategy::Block)
		.constant("Pass", FocusStrategy::Pass)
		.add(flecs::Exclusive);

	world.component<Button>()
		.add(flecs::With, world.component<Interaction>())
		.add(flecs::With, world.component<FocusStrategy>())
		.add(flecs::With, world.component<Node>());

	world.component<Composite>()
		.add(flecs::With, world.component<Image>());

	world.observer<CustomNodeIndex>()
		.event(flecs::OnSet)
		.with(flecs::ChildOf, flecs::Wildcard)
		.with<Node>()
		.each([&world](flecs::entity child, CustomNodeIndex& customIndex) {
			auto parent = child.parent();
			auto children = utils::get_children(parent);

			std::erase(children, child);
			children.insert(children.begin() + customIndex.value, child);
			parent.set_child_order(children.data(), children.size());

			child.remove<CustomNodeIndex>(); // TODO: is it right?
			world.add<UiTreeChanged>();
		});

	world.observer()
		.with(flecs::Disabled)
		.with<Node>().filter()
		.event(flecs::OnAdd)
		.event(flecs::OnRemove)
		.each([&world]() {
			world.add<UiTreeChanged>();
		});

	world.observer<Node>()
		.event(flecs::OnAdd)
		.with(flecs::ChildOf, flecs::Wildcard)
		.each([&world](flecs::entity child, Node& node) {
			world.add<UiTreeChanged>();
		});

	world.observer<RenderLayers, Node>()
		.term_at(1).filter()
		.event(flecs::OnAdd)
		.each([&world](RenderLayers& render_layers, Node& node) {
			render_layers.mask = RenderLayers::UI;
		});

	world.system<Node, WindowModule>()
		.without<Node>().parent()
		.kind(Phases::Update)
		.each([](Node& node, WindowModule& module) {
			glm::ivec2 window_size;

			SDL_GetWindowSize(module.main_window, &window_size.x, &window_size.y);

			if (std::holds_alternative<Node::Fixed>(node.sizing_policy.first)) {
				std::get<Node::Fixed>(node.sizing_policy.first).value = static_cast<float>(window_size.x);
			}
			if (std::holds_alternative<Node::Fixed>(node.sizing_policy.second)) {
				std::get<Node::Fixed>(node.sizing_policy.second).value = static_cast<float>(window_size.y);
			}
		});

	world.system<NodeVector>()
		.with<Node>()
		.without(flecs::ChildOf, flecs::Wildcard) // TODO: SceneRoot in future
		.kind(Phases::PreUpdate)
		.each([&world](flecs::entity e, NodeVector& node_vector) {
			auto index = -1;

			node_vector.sorted_nodes.clear();

			utils::dfs(e, [&index, &node_vector](flecs::entity child) {
				if (!child.enabled()) {
					return;
				}

				child.get_ref<Node>()->stack_index = ++index;
				node_vector.sorted_nodes.emplace_back(child);
			});

			world.remove<UiTreeChanged>();
		});

	world.system<GlobalTransform, Image, Aabb>()
		.kind(Phases::Update)
		.each([](GlobalTransform& transform, Image& image, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + image.texture->get_size();
		});

	world.system<GlobalTransform, Node, Aabb>()
		.with<BackgroundColor>()
		.without<Image>()
		.without<Text>()
		.kind(Phases::Update)
		.each([](GlobalTransform& transform, Node& node, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + node.size;
		});

	world.system<GlobalTransform, TextData, Aabb>()
		.kind(Phases::Update)
		.each([](GlobalTransform& transform, TextData& text_data, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + glm::vec2(text_data.size);
		});

	world.system<const Node, Interaction, const FocusStrategy, const GlobalTransform>("focus interaction")
		.kind(Phases::Update)
		.order_by<Node>([](flecs::entity_t e1, const Node* n1, flecs::entity_t e2, const Node* n2) {
			return (n1->stack_index < n2->stack_index) - (n1->stack_index > n2->stack_index);
		})
		.run([&world](flecs::iter& it) {
			const auto& input = world.get<Input>();

			auto last_focus_strategy = FocusStrategy::Pass;

			while(it.next()) {
				const auto nodes = it.field<const Node>(0);
				const auto interactions = it.field<Interaction>(1);
				const auto foucs_strategies = it.field<const FocusStrategy>(2);
				const auto transforms = it.field<const GlobalTransform>(3);

				for (auto i : it) {
					const auto& node = nodes[i];
					auto& interaction = interactions[i];
					const auto& foucs_strategie = foucs_strategies[i];
					const auto& transform = transforms[i];

					interaction = Interaction::None;

					if (last_focus_strategy == FocusStrategy::Block) {
						continue;
					}

					const auto& mouse_pos = input.mouse.position;
					const auto& pos = transform.translation;
					const auto& size = node.size;

					if (mouse_pos.x >= pos.x && mouse_pos.x <= (pos.x + size.x) && mouse_pos.y >= pos.y && mouse_pos.y <= (pos.y + size.y)) {
						if (input.mouse.left.pressed || input.mouse.left.remain) {
							interaction = Interaction::Clicked;
						}
						else {
							interaction = Interaction::Hovered;
						}

						last_focus_strategy = foucs_strategie;
					}
				}
			}
		});

	world.system<LayoutComposer>()
		.kind(Phases::Update)
		.each([](LayoutComposer& composer) {
			composer.clear();
		});

	world.system<Node*, Node, Text*, TextFont*, TextData*, Transform, LayoutComposer>()
		.term_at(0).parent()
		.cascade()
		.kind(Phases::Update)
		.each([](Node* parent, Node& child, Text* text, TextFont* text_font, TextData* text_data, Transform& transform, LayoutComposer& composer) {
			const auto size = [&] {
				if (text_data) {
					return glm::vec2(text_data->size);
				}

				glm::vec2 size = {};

				if (std::holds_alternative<Node::Fixed>(child.sizing_policy.first)) {
					size.x = std::get<Node::Fixed>(child.sizing_policy.first).value;
				}
				if (std::holds_alternative<Node::Fixed>(child.sizing_policy.second)) {
					size.y = std::get<Node::Fixed>(child.sizing_policy.second).value;
				}

				return size;
			}();

			const auto min_size = [&] {
				std::pair<std::optional<float>, std::optional<float>> size;

				if (text_data) {
					size.first = text_data->min_width;
				}
				else if (std::holds_alternative<Node::Fit>(child.sizing_policy.first)) {
					size.first = std::get<Node::Fit>(child.sizing_policy.first).min;
				}
				else if (std::holds_alternative<Node::Grow>(child.sizing_policy.first)) {
					size.first = std::get<Node::Grow>(child.sizing_policy.first).min;
				}

				if (std::holds_alternative<Node::Fit>(child.sizing_policy.second)) {
					size.second = std::get<Node::Fit>(child.sizing_policy.second).min;
				}
				else if (std::holds_alternative<Node::Grow>(child.sizing_policy.second)) {
					size.second = std::get<Node::Grow>(child.sizing_policy.second).min;
				}

				return size;
			}();

			const auto max_size = [&child] {
				std::pair<std::optional<float>, std::optional<float>> size;

				if (std::holds_alternative<Node::Fit>(child.sizing_policy.first)) {
					size.first = std::get<Node::Fit>(child.sizing_policy.first).max;
				}
				else if (std::holds_alternative<Node::Grow>(child.sizing_policy.first)) {
					size.first = std::get<Node::Grow>(child.sizing_policy.first).max;
				}

				if (std::holds_alternative<Node::Fit>(child.sizing_policy.second)) {
					size.second = std::get<Node::Fit>(child.sizing_policy.second).max;
				}
				else if (std::holds_alternative<Node::Grow>(child.sizing_policy.second)) {
					size.second = std::get<Node::Grow>(child.sizing_policy.second).max;
				}

				return size;
			}();

			auto layout_node = LayoutNode{
				.parent_stack_index = (!parent ? child.stack_index: parent->stack_index) + 1,
				.stack_index = child.stack_index + 1,
				.horizontal = child.grow_direction == Node::GrowDirection::Horizontal,
				.size = size,
				.child_alignment = child.child_alignment,
				.pos = !parent ? transform.translation : glm::vec3{},
				.child_gap = child.child_gap,
				.padding = child.padding,
				.fixed = { std::holds_alternative<Node::Fixed>(child.sizing_policy.first), std::holds_alternative<Node::Fixed>(child.sizing_policy.second) },
				.grow = { std::holds_alternative<Node::Grow>(child.sizing_policy.first), std::holds_alternative<Node::Grow>(child.sizing_policy.second) },
				.fit = { std::holds_alternative<Node::Fit>(child.sizing_policy.first), std::holds_alternative<Node::Fit>(child.sizing_policy.second) },
				.self_alignment = child.self_alignment,
				.min_size = min_size,
				.max_size = max_size
			};

			if (text && text_font && text_data) {
				layout_node.text_data = {
					.text = static_cast<std::string>(*text),
					.font = text_font->handle->get_resource(),
					.font_scale = text_font->font_scale,
				};
			}

			composer.push_node(std::move(layout_node));
		});

	world.system<LayoutComposer>()
		.kind(Phases::Update)
		.each([](LayoutComposer& composer) {
			composer.build();
		});

	world.system<Node, Transform, const LayoutComposer>()
		.kind(Phases::Update)
		.each([](flecs::entity entity, Node& node, Transform& transform, const LayoutComposer& composer) {
			const auto& layout = composer[node.stack_index + 1];

			if (layout.text_data) {
				entity.set<TextComputed>({
					.computed_text = layout.text_data.value().text
				});
			}

			transform.translation = glm::vec3(layout.pos, 0.f);
			node.size = layout.size;
		});

	world.add<NodeVector>();
	world.add<LayoutComposer>();
}
