#include "module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/render_module/module.h"
#include "ecsModule/utils.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "font.h"
#include "layout.h"

#include <algorithm>
#include <vector>

using namespace ps;

void update_clipping(flecs::entity entity, const GlobalTransform& transform, const Node& node, std::optional<SDL_Rect> parent_clip) {
	if (!node.display) {
		parent_clip = SDL_Rect{ static_cast<int>(transform.translation.x), static_cast<int>(transform.translation.y), 0, 0 };

		entity.set<ClipContent>(parent_clip.value());
	}
	else if (node.overflow.first == Node::Overflow::Clip && node.overflow.second == Node::Overflow::Clip) {
		parent_clip = SDL_Rect{
			static_cast<int>(transform.translation.x),
			static_cast<int>(transform.translation.y),
			static_cast<int>(node.size.x),
			static_cast<int>(node.size.y) };

		entity.set<ClipContent>(parent_clip.value());
	}
	else if (parent_clip) {
		entity.set<ClipContent>(parent_clip.value());
	}
	else if (entity.has<ClipContent>()) {
		entity.remove<ClipContent>();
	}

	entity.children([&parent_clip](flecs::entity child) {
		update_clipping(child, child.get<GlobalTransform>(), child.get<Node>(), parent_clip);
	});
}

UiModule::UiModule(flecs::world& world) {
	world.module<UiModule>();

	world.import<TransformModule>();
	world.import<InputModule>();
	world.import<TextModule>();
	world.import<CameraModule>();

	world.component<LayoutComposer>()
		.add(flecs::Singleton);

	world.component<UiTreeChanged>()
		.add(flecs::Singleton);

	world.component<CustomNodeIndex>()
		.member<size_t>("index");

	world.component<BackgroundColor>()
		.is_a<Color>();

	world.component<BorderColor>()
		.is_a<Color>();

	world.component<Node::Grow>()
		.member("min", &Node::Grow::min)
		.member("max", &Node::Grow::max);

	world.component<Node::Fit>()
		.member("min", &Node::Grow::min)
		.member("max", &Node::Grow::max);

	world.component<NodeIndex>()
		.member("dfs", &NodeIndex::dfs)
		.member("bfs", &NodeIndex::bfs);

	world.component<Node::Fixed>()
		.member("value", &Node::Fixed::value);

	world.component<Node::GrowDirection>()
		.constant("Vertical", Node::GrowDirection::Vertical)
		.constant("Horizontal", Node::GrowDirection::Horizontal);

	world.component<Node>()
		.member("sizing_policy", &Node::sizing_policy)
		.member("self_alignment", &Node::self_alignment)
		.member("size", &Node::size)
		.member("child_alignment", &Node::child_alignment)
		.member("child_gap", &Node::child_gap)
		.member("margin", &Node::margin)
		.member("padding", &Node::padding)
		.member("grow_direction", &Node::grow_direction)
		.member("display", &Node::display)
		.member("absolute", &Node::absolute)
		.member("border_radius", &Node::border_radius)
		.member("border_width", &Node::border_width)
		.add(flecs::With, world.component<NodeIndex>())
		.add(flecs::With, world.component<RenderLayers>())
		.add(flecs::With, world.component<Aabb>())
		.add(flecs::With, world.component<Visible2d>())
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<BackgroundColor>())
		.add(flecs::With, world.component<BorderColor>())
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
		.add(flecs::Exclusive)
		.add(flecs::Sparse); //TODO: think about it

	world.component<FocusStrategy>()
		.constant("Block", FocusStrategy::Block)
		.constant("Pass", FocusStrategy::Pass)
		.add(flecs::Exclusive)
		.add(flecs::Sparse); //TODO: think about it

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
		.each([&world](flecs::iter& it, size_t i) {
			it.entity(i).children([&it](flecs::entity child) {
				it.event() == flecs::OnAdd ? child.disable() : child.enable();
			});
			world.add<UiTreeChanged>();
		});

	world.observer<Node>()
		.event(flecs::OnAdd)
		.event(flecs::OnRemove)
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

			if (std::holds_alternative<Node::Grow>(node.sizing_policy.first)) {
				std::get<Node::Grow>(node.sizing_policy.first).min = static_cast<float>(window_size.x);
			}
			if (std::holds_alternative<Node::Grow>(node.sizing_policy.second)) {
				std::get<Node::Grow>(node.sizing_policy.second).min = static_cast<float>(window_size.y);
			}
		});

	world.system()
		.with<UiTreeChanged>()
		.with<NodeIndex>()
		.without<NodeIndex>().parent()
		//.without(flecs::ChildOf, flecs::Wildcard) // TODO: SceneRoot in future
		.kind(Phases::Update)
		.run([&world](flecs::iter& it) {
			size_t dfs_index = 0;
			size_t bfs_index = 0;

			std::vector<flecs::entity> roots;

			while (it.next()) {
				for (auto i : it) {
					roots.push_back(it.entity(i));
				}
			}

			utils::dfs(roots, [&dfs_index](flecs::entity child) {
				if (!child.enabled()) {
					return;
				}

				child.get_mut<NodeIndex>().dfs = dfs_index++;
			});

			utils::bfs(roots, [&bfs_index](flecs::entity child) {
				if (!child.enabled()) {
					return;
				}

				child.get_mut<NodeIndex>().bfs = bfs_index++;
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

	auto interactions_query = world.query_builder<const Node, const GlobalTransform, const NodeIndex, const FocusStrategy, Interaction>()
		.with<const ClipContent>().optional()
		.build();

	world.system<Input>("focus interaction new")
		.kind(Phases::Update)
		.each([interactions_query](Input& input) {
			std::vector<TempInteraction> interactions;

			interactions_query.run([&input, &interactions](flecs::iter& it) {
				while(it.next()) {
					const auto node_field = it.field<const Node>(0);
					const auto transform_field = it.field<const GlobalTransform>(1);
					const auto stack_index_field = it.field<const NodeIndex>(2);

					for (auto i : it) {
						const auto& node = node_field[i];
						const auto& transform = transform_field[i];
						const auto& stack_index = stack_index_field[i];
						const auto& focus_strategy = it.field_at<const FocusStrategy>(3, i);
						auto& interaction = it.field_at<Interaction>(4, i);

						interaction = Interaction::None;

						const auto& mouse_pos = input.mouse.position;
						const auto& pos = transform.translation;
						const auto& size = node.size;

						if (mouse_pos.x >= pos.x && mouse_pos.x <= (pos.x + size.x) && mouse_pos.y >= pos.y && mouse_pos.y <= (pos.y + size.y)) {
							if (it.is_set(5)) {
								auto &clip_content = it.field_at<const ClipContent>(5, i);

								if (clip_content.w == 0 && clip_content.h == 0) {
									continue;
								}
							}

							interactions.emplace_back(it.entity(i), stack_index.dfs, focus_strategy);
						}
					}
				}
			});

			if (interactions.empty()) {
				return;
			}

			std::ranges::sort(interactions, std::greater{}, &TempInteraction::stack_index);

			for (const auto& interaction : interactions) {
				interaction.entity.get_mut<Interaction>() = input.mouse.left.pressed ? Interaction::Clicked : Interaction::Hovered;

				if (interaction.focus_strategy == FocusStrategy::Block) {
					break;
				}
			}
		});

	world.system<LayoutComposer>()
		.kind(Phases::Update)
		.each([](LayoutComposer& composer) {
			composer.clear();
		});

	world.system<NodeIndex*, Node, NodeIndex, Text*, TextFont*, TextData*, Transform>()
		.term_at(0).parent()
		.kind(Phases::Update)
		.each([&world](flecs::entity entity, NodeIndex* parent, Node& node, NodeIndex& index, Text* text, TextFont* text_font, TextData* text_data, Transform& transform) {
			auto& composer = world.get_mut<LayoutComposer>();
			const auto size = [&] {
				if (text_data) {
					return glm::vec2(text_data->size);
				}

				glm::vec2 size = {};

				if (std::holds_alternative<Node::Fixed>(node.sizing_policy.first)) {
					size.x = std::get<Node::Fixed>(node.sizing_policy.first).value;
				}
				if (std::holds_alternative<Node::Fixed>(node.sizing_policy.second)) {
					size.y = std::get<Node::Fixed>(node.sizing_policy.second).value;
				}

				return size;
			}();

			const auto min_size = [&] {
				std::pair<std::optional<float>, std::optional<float>> size;

				if (text_data) {
					size.first = text_data->min_width;
				}
				else if (std::holds_alternative<Node::Fit>(node.sizing_policy.first)) {
					size.first = std::get<Node::Fit>(node.sizing_policy.first).min;
				}
				else if (std::holds_alternative<Node::Grow>(node.sizing_policy.first)) {
					size.first = std::get<Node::Grow>(node.sizing_policy.first).min;
				}

				if (std::holds_alternative<Node::Fit>(node.sizing_policy.second)) {
					size.second = std::get<Node::Fit>(node.sizing_policy.second).min;
				}
				else if (std::holds_alternative<Node::Grow>(node.sizing_policy.second)) {
					size.second = std::get<Node::Grow>(node.sizing_policy.second).min;
				}

				return size;
			}();

			const auto max_size = [&node] {
				std::pair<std::optional<float>, std::optional<float>> size;

				if (std::holds_alternative<Node::Fit>(node.sizing_policy.first)) {
					size.first = std::get<Node::Fit>(node.sizing_policy.first).max;
				}
				else if (std::holds_alternative<Node::Grow>(node.sizing_policy.first)) {
					size.first = std::get<Node::Grow>(node.sizing_policy.first).max;
				}

				if (std::holds_alternative<Node::Fit>(node.sizing_policy.second)) {
					size.second = std::get<Node::Fit>(node.sizing_policy.second).max;
				}
				else if (std::holds_alternative<Node::Grow>(node.sizing_policy.second)) {
					size.second = std::get<Node::Grow>(node.sizing_policy.second).max;
				}

				return size;
			}();

			auto layout_node = LayoutNode{
				.parent_bfs_index = parent ? parent->bfs + 1 : index.bfs + 1,
				.bfs_index = index.bfs + 1,
				.horizontal = node.grow_direction == Node::GrowDirection::Horizontal,
				.display = node.display,
				.absolute = node.absolute,
				.size = size,
				.child_alignment = node.child_alignment,
				.pos = transform.translation,
				.child_gap = node.child_gap,
				.padding = node.padding,
				.fixed = { std::holds_alternative<Node::Fixed>(node.sizing_policy.first), std::holds_alternative<Node::Fixed>(node.sizing_policy.second) },
				.grow = { std::holds_alternative<Node::Grow>(node.sizing_policy.first), std::holds_alternative<Node::Grow>(node.sizing_policy.second) },
				.fit = { std::holds_alternative<Node::Fit>(node.sizing_policy.first), std::holds_alternative<Node::Fit>(node.sizing_policy.second) },
				.self_alignment = node.self_alignment,
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

	world.system<Node, NodeIndex, Transform, const LayoutComposer>()
		.kind(Phases::Update)
		.each([](flecs::entity entity, Node& node, NodeIndex& index, Transform& transform, const LayoutComposer& composer) {
			const auto& layout = composer[index.bfs + 1];

			if (layout.text_data) {
				entity.set<TextComputed>({
					.computed_text = layout.text_data.value().text
				});
			}

			transform.translation = glm::vec3(layout.pos, 0.f);
			node.size = layout.size;
		});

	world.add<LayoutComposer>();
}
