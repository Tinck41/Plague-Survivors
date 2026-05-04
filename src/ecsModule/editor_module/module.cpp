#include "module.h"
#include "components.h"

#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/editor_module/docking.h"
#include "ecsModule/editor_module/functions.h"
#include "ecsModule/editor_module/window.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/utils.h"
#include "ecsModule/windowModule/module.h"
#include "ext/quaternion_common.hpp"
#include "spdlog/spdlog.h"
#include "glaze/glaze.hpp"
#include "utils/visit.h"
#include "gtx/compatibility.hpp"

using namespace ps;

template <typename T, typename Callable>
void for_each_field_with_name(T&& value, Callable&& cb) {
    constexpr auto N = glz::reflect<std::decay_t<T>>::size;
    [&]<size_t... I>(std::index_sequence<I...>) {
        ([&] {
            constexpr auto name = glz::reflect<std::decay_t<T>>::keys[I];
            auto& field = glz::get_member(value, glz::get<I>(glz::to_tie(value)));
            cb(name, field);
        }(), ...);
    }(std::make_index_sequence<N>{});
}

EditorModule::EditorModule(flecs::world& world) {
	world.module<EditorModule>();

	world.component<EditorNode>()
		.add(flecs::With, world.component<Node>());

	world.component<EditorRoot>()
		.add(flecs::With, world.component<EditorNode>());

	world.component<Hierarchy>()
		.add(flecs::With, world.component<EditorNode>());

	world.component<Viewport>()
		.add(flecs::With, world.component<EditorNode>());

	world.component<Inspector>()
		.add(flecs::With, world.component<EditorNode>());

	world.component<Console>()
		.add(flecs::With, world.component<EditorNode>());

	world.component<TreeNodePart>();
	world.component<TreeNode>()
		.add(flecs::With, world.component<EditorNode>());

	world.component<EditorWindow>()
		.add(flecs::With, world.component<EditorNode>());
	world.component<WindowContent>()
		.add(flecs::With, world.component<EditorNode>());
	world.component<WindowTitlebar>()
		.add(flecs::With, world.component<EditorNode>());
	world.component<WindowTab>()
		.add(flecs::With, world.component<Button>())
		.add(flecs::With, world.component<EditorNode>());
	world.component<WindowCollapseButton>()
		.add(flecs::With, world.component<Button>())
		.add(flecs::With, world.component<EditorNode>());
	world.component<WindowResizeButton>()
		.add(flecs::With, world.component<Button>())
		.add(flecs::With, world.component<EditorNode>());

	world.component<NowDragged>();

	world.component<DockedTo>()
		.add(flecs::PairIsTag);

	world.component<Palette>()
		.member("bg", &Palette::bg)
		.member("text", &Palette::text)
		.member("highlight", &Palette::highlight)
		.member("select", &Palette::select);

	world.component<EditorSettings>()
		.member("palette", &EditorSettings::palette)
		.add(flecs::Singleton);

	world.import<DockingModule>();

	world.system()
		.kind(Phases::OnStart)
		.each([&world] {
			auto editor_root = world.entity("editor_root")
				.add<EditorRoot>()
				.set<Node>({
					.sizing_policy = { Node::Grow{}, Node::Grow{} },
				});

			auto [main_window, main_content] = create_window(world, {
				.name = "main",
				.flags = WindowFlags::NoTitlebar | WindowFlags::NoResize | WindowFlags::NoResize | WindowFlags::NoMove | WindowFlags::NoDocking,
			});

			auto dockspace = create_dockspace(world, main_window, "main_dockspace");

			dockspace.child_of(main_content);

			auto [window, content] = create_window(world, {
				.name = "test",
				.flags = WindowFlags::None,
				.size = glm::vec2{ 300, 300 },
			});

			auto [new_window, new_content] = create_window(world, {
				.name = "test2",
				.flags = WindowFlags::None,
				.size = glm::vec2{ 300, 300 },
			});

			main_window.child_of(editor_root);
			window.child_of(editor_root);
			new_window.child_of(editor_root);
		});

	world.system<Input>()
		.kind(Phases::Update)
		.each([&world](Input& input) {
			if (input.keys[Key::F1].released) {
				auto window_data = create_window(world, {
					.name = std::format("window_{}", ecs_get_world_info(world)->frame_count_total),
					.flags = WindowFlags::None,
					.size = glm::vec2{ 300, 300 },
				});

				window_data.first.child_of(world.entity("editor_root"));
			}
		});

	//new_window.child_of(main_content);

	//auto project_namespace = world.lookup("ps");

	//world.system()
	//	.with(flecs::Module)
	//	.with(flecs::ChildOf, project_namespace)
	//	.kind(Phases::OnStart)
	//	.each([&world, new_content](flecs::entity entity) {
	//		auto hierarchy = world.entity("hierarchy")
	//			.child_of(new_content)
	//			.set<Hierarchy>({
	//			})
	//			.set<BackgroundColor>(Color::from_hex("#282828"))
	//			.set<Node>({
	//				.sizing_policy = { Node::Grow{ .min  = 50.f }, Node::Grow{} },
	//				.grow_direction = Node::GrowDirection::Vertical,
	//			});

	//		create_tree_node(world, entity).child_of(hierarchy);
	//	});


	auto child_query = world.query_builder()
		.with(flecs::ChildOf, "$parent")
		.build();

	world.system<BackgroundColor, TreeNode, Interaction, EditorSettings>("tree picker")
		.kind(Phases::Update)
		.each([child_query](flecs::entity entity, BackgroundColor& color, TreeNode& item, Interaction& interaction, EditorSettings& settings) {
			auto world = entity.world();
			switch (interaction) {
				case Interaction::None: {
					color = TRANSPARENT;
					break;
				}
				case Interaction::Hovered: {
					color = settings.palette.highlight;
					break;
				}
				case Interaction::Clicked: {
					if (item.selected) {
						entity.world().query_builder()
							.with(flecs::ChildOf, item.container)
							.build()
							.run([](flecs::iter& it) {
								while (it.next()) {
									for (auto i : it) {
										it.entity(i).destruct();
									}
								}
							});

						item.selected = false;
						entity.remove<TreeNodeSelected>();
					}
					else {
						child_query
							.set_var("parent", item.entity)
							.run([entity, item](flecs::iter& it) {
								auto world = entity.world();
								while (it.next()) {
									for (auto i : it) {
										create_tree_node(world, it.entity(i)).child_of(item.container);
									}
								}
							});

						item.selected = true;
						entity.add<TreeNodeSelected>();
					}
					break;
				}
			}
		});

	auto collapse_target_query = world.query_builder()
		.with<CollapseTarget>("$collapse_target").src("$collapse_button")
		.with<Node>().src("$collapse_target").inout()
		.build();

	world.system<Node, EditorWindow, Interaction>("window collapse button")
		.with<CollapseButton>().second("$collapse_button")
		.term_at(2).src("$collapse_button")
		.kind(Phases::Update)
		.each([collapse_target_query](flecs::iter& it, size_t i, Node& window_node, EditorWindow& window, Interaction& interaction) {
			if (interaction == Interaction::Clicked) {
				auto collapse_button = it.get_var("collapse_button");

				collapse_target_query
					.set_var("collapse_button", collapse_button)
					.run([](flecs::iter& it) {
						while (it.next()) {
							auto node_field = it.field<Node>(1);
							node_field[0].display = !node_field[0].display;
						}
					});

				if (!window.collapsed) {
					auto& sizing_x = std::get<Node::Fit>(window_node.sizing_policy.first);
					auto& sizing_y = std::get<Node::Fit>(window_node.sizing_policy.second);

					window.size = glm::vec2(sizing_x.min.value_or(0.f), sizing_y.min.value_or(0.f));
					sizing_y.min = 0.f;

					window.collapsed = true;
				}
				else {
					std::get<Node::Fit>(window_node.sizing_policy.second).min = window.size.y;

					window.collapsed = false;
				}
			}
		});

	world.system<Interaction, Input>("window resize button")
		.with<ResizeTarget>().second("$resize_target")
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, Interaction& interaction, Input& input) {
			if (interaction == Interaction::Clicked) {
				auto resize_target = it.get_var("resize_target");

				resize_target.add<TrackCursor>();
			}
		});

	//world.system<Interaction, Input>("window drag button")
	//	.with<WindowTab>()
	//	.kind(Phases::Update)
	//	.each([](flecs::entity entity, Interaction& interaction, Input& input) {
	//		if (interaction == Interaction::Clicked) {
	//			auto parent = entity.parent().parent();
	//			auto offset = glm::vec2(parent.get<GlobalTransform>().translation) - input.mouse.position;

	//			parent.set<TrackCursor>({offset});
	//		}
	//	});

	world.system<Node, GlobalTransform, Input>("resize window")
		.with<ResizeTarget>().second("$resize_target")
		.with<TrackCursor>().src("$resize_target")
		.without<NowDragged>().src("$resize_target")
		.term_at(0).src("$resize_target")
		.term_at(1).src("$resize_target")
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, Node& node, GlobalTransform& transform, Input& input) {
			const auto pos = glm::vec2(transform.translation);
			const auto size = glm::vec2(input.mouse.position - pos);

			const auto set_size = [](Node::SizePolicy& policy, float value) {
				visit(policy, visitors{
					[&](Node::Fit& fit) {
						fit.min = value;
					},
					[&](Node::Grow& grow) {
						grow.min = value;
					},
					[&](Node::Fixed& fixed) {
						fixed.value = value;
					}
				});
			};

			set_size(node.sizing_policy.first, size.x);
			set_size(node.sizing_policy.second, size.y);

			if (input.mouse.left.released) {
				it.get_var("resize_target").remove<TrackCursor>();
			}
		});

	world.system<Interaction, Input>("close window button")
		.with<CloseTarget>("$close_target")
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, Interaction& interaction, Input& input) {
			if (interaction == Interaction::Hovered && input.mouse.left.released) {
				it.get_var("close_target").destruct();
			}
		});

	world.system<Interaction, GlobalTransform, Input, DockTree>("drag window")
		.with<DragTarget>("$drag_target")
		.term_at(1).src("$drag_target")
		.kind(Phases::Update)
		.immediate() // todo: investigate crash due to world is in multithreaded mode while calling utils::insert_child_back()
		.each([](flecs::iter& it, size_t i, Interaction& interaction, GlobalTransform& transform, Input& input, DockTree& tree) {
			if (interaction == Interaction::Clicked) {
				const auto drag_target = it.get_var("drag_target");
				const auto entity = it.entity(i);
				const auto editor_root = it.world().lookup("editor_root");

				drag_target.add<NowDragged>();

				utils::insert_child_back(editor_root, drag_target);

				entity.set<TrackCursor>({
					.offset = input.mouse.position - glm::vec2(transform.translation),
				});

				if (drag_target.has<DockNodeRef>()) {
					tree.undock_window(drag_target, drag_target.get<DockNodeRef>().id);
				}
			}
		});

	world.system<TrackCursor, Node, Transform, Interaction, Input>("window drag")
		.with<DragTarget>("$drag_target")
		.with<NowDragged>().src("$drag_target")
		.term_at(1).src("$drag_target")
		.term_at(2).src("$drag_target")
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, TrackCursor& track, Node& node, Transform& transform, Interaction& interaction, Input& input) {
			transform.translation = glm::vec3(input.mouse.position - track.offset, 0.f);
			node.absolute = true;

			if (input.mouse.left.released) {
				it.entity(i).remove<TrackCursor>();
				it.entity(i).remove<NowDragged>();
				it.entity(i).set(FocusStrategy::Block);
			}
		});

	//world.system<TrackCursor, Transform, Input>("drag window")
	//	.with<EditorWindow>()
	//	.kind(Phases::Update)
	//	.each([](flecs::entity entity, TrackCursor& track, Transform& transform, Input& input) {
	//		transform.translation = glm::vec3(input.mouse.position + track.offset, 0.f);

	//		if (input.mouse.left.released) {
	//			entity.remove<TrackCursor>();
	//		}
	//	});

	world.system<Node, WindowModule>()
		.with<EditorWindow>()
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

	world.set<EditorSettings>({
		.palette = dark,
	});
}
