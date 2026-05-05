#include "docking.h"
#include "ecsModule/common.h"
#include "ecsModule/editor_module/window.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/transformModule/module.h"
#include "spdlog/spdlog.h"
#include "utils/visit.h"

#include <algorithm>
#include <queue>

using namespace ps;

void DockTree::add_dockspace(flecs::entity& dockspace) {
	auto& node = create_root(glm::vec2(), glm::vec2());

	dockspace.set<DockNodeRef>({ node.id });

	node.dockspace = true;
	node.central_node_id = node.id;
}

void DockTree::dock_window(flecs::entity source, const Node& source_node, const DockingTarget& docking_target, SplitAxis split_axis, DockSide dock_side) {
	const auto target_id = visit(docking_target, visitors{
		[&](const WindowTarget& target) {
			return create_root(target.position, target.size).id;
		},
		[](const NodeTarget& target) {
			return target.id;
		}
	});

	auto& target_node = nodes[target_id];

	const auto source_child_id = static_cast<std::uint8_t>(dock_side);

	const auto windows = target_node.windows;
	const auto active_window = target_node.active_window;
	const auto children = target_node.children;
	const auto prev_split_axis = target_node.split_axis;

	split_node(target_node.id, split_axis);

	auto& leaf_node = nodes[target_node.children[source_child_id]];
	auto& other_node = nodes[target_node.children[1 - source_child_id]];

	other_node.children = children;
	other_node.split_axis = prev_split_axis;
	other_node.windows = windows;
	other_node.active_window = active_window;

	for (auto& window : windows) {
		window.set<DockNodeRef>({ other_node.id });
	}

	for (const auto child : children) {
		if (child == 0) {
			continue;
		}

		nodes[child].parent_id = other_node.id;
	}

	leaf_node.windows.push_back(source);
	leaf_node.active_window = leaf_node.windows.size() - 1;
	leaf_node.aspect_ratio = split_axis == SplitAxis::Horizontal
		? source_node.size.y / target_node.size.y
		: source_node.size.x / target_node.size.x;
	leaf_node.aspect_ratio = std::min(leaf_node.aspect_ratio, 0.5f);

	other_node.aspect_ratio = 1.f - leaf_node.aspect_ratio;

	source.set<DockNodeRef>({ leaf_node.id });

	if (std::holds_alternative<WindowTarget>(docking_target)) {
		auto& window = std::get<WindowTarget>(docking_target).window;

		other_node.windows.push_back(window);
		other_node.active_window = other_node.windows.size() - 1;

		window.set<DockNodeRef>({ other_node.id });
	}

	update_buttons(get_root_node(leaf_node.id));

	update();
}

void DockTree::undock_window(flecs::entity window, DockNodeId node_id) {
	if (!nodes.contains(node_id)) {
		return;
	}

	window.remove<DockNodeRef>();

	auto it = std::ranges::find(nodes[node_id].windows, window);

	if (it != nodes[node_id].windows.end()) {
		if (it->has<ResizeButton>(flecs::Any)) {
			const auto resize_button = it->target<ResizeButton>(0);

			resize_button.enable();
			resize_button.add<ResizeTarget>(*it);

			it->add<DragTarget>(*it);
		}

		nodes[node_id].windows.erase(it);

		update_buttons(get_root_node(node_id));
	}

	if (nodes[node_id].windows.empty() && nodes[node_id].parent_id != 0) {
		auto& parent = nodes[nodes[node_id].parent_id];

		if (!parent.dockspace) {
			for (const auto child_id : parent.children) {
				auto& child = nodes[child_id];

				if (child_id == node_id) {
					child.parent_id = 0;
					remove_ids.push_back(child_id);
				}
				else {
					child.parent_id = parent.parent_id;
					child.aspect_ratio = parent.aspect_ratio;
					child.size = parent.size;
					child.position = parent.position;

					if (child.parent_id == 0) {
						root_node_ids.push_back(child.id);
					}
					else {
						auto& grandparent = nodes[child.parent_id];

						for (size_t i = 0; i < grandparent.children.size(); ++i) {
							if (grandparent.children[i] == parent.id) {
								grandparent.children[i] = child.id;
							}
						}
					}
				}
			}

			parent.children = { 0, 0 };

			remove_ids.push_back(parent.id);
		}
		else {
			const auto children = parent.children;

			for (const auto child_id : children) {
				auto& child = nodes[child_id];

				if (child_id != node_id) {
					parent.split_axis = child.split_axis;
					parent.children = child.children;

					for (const auto granchild_id : child.children) {
						auto& granchild = nodes[granchild_id];

						granchild.parent_id = parent.id;
					}
				}

				remove_ids.push_back(child_id);
			}
		}
	}
}

void DockTree::update() {
	for (auto root : root_node_ids) {
		bfs(root, [&](DockNode& node) {
			if (!node.dockspace && node.parent_id == 0 && !has_children(node.id) && node.windows.size() <= 1) {
				remove_ids.push_back(node.id);
			}

			for (size_t i = 0; i < node.children.size(); ++i) {
				if (node.children[i] == 0) {
					continue;
				}

				const auto multiplier = static_cast<float>(i);
				auto& child = nodes[node.children[i]];

				child.position = node.position;

				if (node.split_axis == SplitAxis::Horizontal) {
					child.size.x = node.size.x;
					child.size.y = node.size.y * child.aspect_ratio;
					child.position.y += node.size.y * multiplier - child.size.y * multiplier;
				}
				else {
					child.size.x = node.size.x * child.aspect_ratio;
					child.size.y = node.size.y;
					child.position.x += node.size.x * multiplier - child.size.x * multiplier;
				}
			}
		});
	}
}

void DockTree::cleanup() {
	for (const auto id : remove_ids) {
		auto it = std::ranges::find(root_node_ids, id);

		for (const auto window : nodes[id].windows) {
			if (window.has<ResizeButton>(flecs::Any)) {
				const auto resize_button = window.target<ResizeButton>(0);

				resize_button.enable();
				resize_button.add<ResizeTarget>(window);

				window.add<DragTarget>(window);
			}

			window.remove<DockNodeRef>();
		}

		nodes.erase(id);
		if (it != root_node_ids.end()) {
			root_node_ids.erase(it);
		}
	}

	remove_ids.clear();
}

DockNode& DockTree::get_node(DockNodeId id) {
	return nodes[id];
}

DockNode& DockTree::get_root_node(DockNodeId id) {
	auto current_id = id;

	while (nodes[current_id].parent_id != 0) {
		current_id = nodes[current_id].parent_id;
	}

	return nodes[current_id];
}

const std::vector<DockNodeId>& DockTree::get_root_nodes() const {
	return root_node_ids;
}

void DockTree::remove_child(DockNodeId parent_id, DockNodeId child_id) {
	auto& parent = nodes[parent_id];
	const auto children = parent.children;

	for (size_t i = 0; i < children.size(); ++i) {
		if (parent.children[i] == 0) {
			continue;
		}

		auto& child = nodes[parent.children[i]];

		if (child.id == child_id) {
			parent.children[i] = 0;
			child.parent_id = 0;
		}
		else {
			child.aspect_ratio = 1.f;
		}
	}

	remove_ids.push_back(child_id);
}

void DockTree::update_buttons(DockNode& root) {
	flecs::entity far_rigth_window;

	dfs(root.id, [&](DockNode& node) {
		if (node.windows.empty()) {
			return;
		}

		for (const auto window : node.windows) {
			window.target<ResizeButton>(0).disable();

			if (!root.dockspace) {
				window.set<DragTarget, DockNodeRef>(root.id);
			}
			else {
				window.remove<DragTarget>(flecs::Wildcard);
			}
		}

		far_rigth_window = node.windows[node.active_window];
	});

	if (!root.dockspace && far_rigth_window.is_valid()) {
		const auto resize_button = far_rigth_window.target<ResizeButton>(0);

		resize_button.enable();
		resize_button.set<ResizeTarget, DockNodeRef>(root.id);
	}
}

bool DockTree::has_children(DockNodeId ndoe_id) {
	return nodes[ndoe_id].children[0] != 0 || nodes[ndoe_id].children[1] != 0;
}

void DockTree::bfs(DockNodeId id, const std::function<void(DockNode&)>& callback) {
	std::queue<DockNodeId> queue;

	queue.push(id);

	while (!queue.empty()) {
		const auto id = queue.front();
		queue.pop();

		callback(nodes[id]);

		for (const auto child_id : nodes[id].children) {
			if (child_id != 0) {
				queue.push(child_id);
			}
		}
	}
}

void DockTree::dfs(DockNodeId id, const std::function<void(DockNode&)>& callback) {
	auto& node = nodes[id];

	callback(node);

	for (const auto child_id : node.children) {
		if (child_id != 0) {
			dfs(child_id, callback);
		}
	}
}

void DockTree::reverse_dfs(DockNodeId id, const std::function<void(DockNode&)>& callback) {
	auto& node = nodes[id];

	for (const auto child_id : node.children) {
		if (child_id != 0) {
			reverse_dfs(child_id, callback);
		}
	}

	callback(node);
}

DockNode& DockTree::create_root(glm::vec2 position, glm::vec2 size) {
	const auto id = ++next_id;
	auto& node = nodes[id];

	node.id = id;
	node.size = size;
	node.position = position;

	root_node_ids.push_back(id);

	return node;
}

std::pair<DockNode&, DockNode&> DockTree::split_node(DockNodeId id, SplitAxis split_axis) {
	const auto left_id = ++next_id;
	const auto right_id = ++next_id;

	auto& node = nodes[id];
	auto& left = nodes[left_id];
	auto& right = nodes[right_id];

	left.parent_id = id;
	left.id = left_id;
	left.aspect_ratio = 0.5f;

	right.parent_id = id;
	right.id = right_id;
	right.aspect_ratio = 0.5f;

	node.children = { left.id, right.id };
	node.split_axis = split_axis;
	node.windows.clear();

	return { left, right };
}

DockingModule::DockingModule(flecs::world& world) {
	world.module<DockingModule>();

	world.component<DockNodeRef>()
		.member("id", &DockNodeRef::id);

	world.component<DockData>()
		.member("target", &DockData::target)
		.member("split_axis", &DockData::split_axis)
		.member("dock_side", &DockData::dock_side);

	world.component<SplitAxis>()
		.constant("Horizontal", SplitAxis::Horizontal)
		.constant("Vertical", SplitAxis::Vertical);

	world.component<DockSide>()
		.constant("TopLeft", DockSide::TopLeft)
		.constant("BotRight", DockSide::BotRight);

	world.component<DockTree>()
		.member("destroy", &DockTree::destroy)
		.add(flecs::Singleton);

	using ResizeNodeTarget = flecs::pair<ResizeTarget, DockNodeRef>;
	using DragNodeTarget = flecs::pair<DragTarget, DockNodeRef>;

	world.observer<DockNodeRef, DockTree>()
		.event(flecs::OnRemove)
		.each([](flecs::entity entity, DockNodeRef& ref, DockTree& tree) {
			tree.undock_window(entity, ref.id);
		});

	world.system<GlobalTransform, Node, Input, DockTree>()
		.with<DockOptionsNode>()
		.kind(Phases::Update)
		.each([](flecs::entity entity, GlobalTransform& transform, Node& node, Input& input, DockTree& tree) {
			if (!tree.destroy) {
				return;
			}
			const auto& mouse_pos = input.mouse.position;

			if (mouse_pos.x > transform.translation.x + node.size.x || mouse_pos.x < transform.translation.x ||
				mouse_pos.y > transform.translation.y + node.size.y || mouse_pos.y < transform.translation.y || !input.mouse.left.remain) {
				entity.destruct();
			}
		});

	auto floating_window_query = world.query_builder<GlobalTransform, Node>()
		.with<EditorWindow>()
		.with<DockingEnabled>()
		.without<DockNodeRef>()
		.without<TrackDrag>()
		.build();

	world.system<Input, DockTree>()
		.with<EditorWindow>()
		.with<TrackDrag>()
		.without<DockNodeRef>()
		.kind(Phases::Update)
		.each([floating_window_query](flecs::entity entity, Input& input, DockTree& tree) {
			const auto& mouse_pos = input.mouse.position;
			const auto& root_nodes = tree.get_root_nodes();
			auto world = entity.world();

			bool collision_detected = false;

			floating_window_query.run([&mouse_pos, &input, &world, &collision_detected](flecs::iter& it) {
				while(it.next()) {
					auto transform_field = it.field<GlobalTransform>(0);
					auto node_field = it.field<Node>(1);

					for (auto i : it) {
						auto entity = it.entity(i);
						auto& transform = transform_field[i];
						auto& node = node_field[i];

						if (mouse_pos.x >= transform.translation.x && mouse_pos.x <= transform.translation.x + node.size.x &&
							mouse_pos.y >= transform.translation.y && mouse_pos.y <= transform.translation.y + node.size.y) {
							if (!entity.has<DockOptions>(flecs::Any)) {
								auto dock_preview_node = create_dockspace_inner_options(world, DockNode{
									.windows = { entity },
									.size = node.size,
									.position = transform.translation,
								});

								entity.add<DockOptions>(dock_preview_node);
							}

							collision_detected = true;

							it.fini();

							return;
						}
					}
				}
			});

			std::vector<DockNodeId> collision_ids;

			for (const auto root_id : root_nodes | std::ranges::views::reverse) {
				if (!collision_ids.empty()) {
					collision_detected = true;
				}

				tree.dfs(root_id, [&](DockNode& node) {
					if (collision_detected) {
						if (node.options_entity.is_valid()) {
							node.options_entity.destruct();
						}

						return;
					}

					if (node.id == 0) {
						return;
					}

					if (mouse_pos.x >= node.position.x && mouse_pos.x <= node.position.x + node.size.x &&
						mouse_pos.y >= node.position.y && mouse_pos.y <= node.position.y + node.size.y) {
						collision_ids.push_back(node.id);
					}
				});
			}

			if (!collision_ids.empty()) {
				auto front = collision_ids.front();
				auto back = collision_ids.back();

				auto& first_node = tree.get_node(front);

				if (!first_node.options_entity.is_valid()) {
					first_node.options_entity = create_dockspace_outer_options(world, first_node);
				}

				if (back != front) {
					auto& second_node = tree.get_node(back);

					if (!second_node.options_entity.is_valid()) {
						second_node.options_entity = create_dockspace_inner_options(world, second_node);
					}
				}

				return;
			}
		});

	auto docking_options_query = world.query_builder<const DockData, const Interaction>()
		.with<DockingOption>()
		.build();

	world.system<Node, Input>()
		.with<EditorWindow>()
		.with<TrackDrag>()
		.kind(Phases::Update)
		.each([docking_options_query](flecs::entity entity, Node& node, Input& input) {
			docking_options_query
				.run([&input, &entity, &node](flecs::iter& it) {
					while (it.next()) {
						auto world = it.world();
						auto dock_data_field = it.field<const DockData>(0);

						for (auto i : it) {
							auto option_entity = it.entity(i);
							const auto& dock_data = dock_data_field[i];
							const auto& interaction = it.field_at<const Interaction>(1, i);

							if (interaction == Interaction::Hovered) {
								if (input.mouse.left.released) {
									world.get_ref<DockTree>()->dock_window(
										entity,
										node,
										dock_data.target,
										dock_data.split_axis,
										dock_data.dock_side
									);
								}
								else if (input.mouse.left.remain && !option_entity.has<DockPreview>(flecs::Any)) {
									const auto preview_entity = create_dockspace_preview(world, node.size, dock_data.split_axis, dock_data.dock_side);
									const auto preview_holder = dock_data.dock_options_container.lookup(std::format("{}_preview_holder", dock_data.dock_options_container.name().c_str()).c_str());

									preview_entity.child_of(preview_holder);

									option_entity.add<DockPreview>(preview_entity);
								}
							}
							else {
								if (option_entity.has<DockPreview>(flecs::Any)) {
									option_entity.target<DockPreview>(0).destruct();
									option_entity.remove<DockPreview>(flecs::Any);
								}
							}
						}
					}
				});
		});

	world.system<ResizeNodeTarget, TrackResize, Input, DockTree>()
		.kind(Phases::Update)
		.each([](flecs::entity entity, ResizeNodeTarget resize_target, TrackResize track_cursor, Input& input, DockTree& tree) {
			auto& node = tree.get_node(resize_target->id);

			node.size += input.mouse.position - track_cursor->origin;
		});

	world.system<DragNodeTarget, TrackDrag, Input, DockTree>("docked window drag")
		.kind(Phases::Update)
		.each([](flecs::entity entity, DragNodeTarget drag_target, TrackDrag track_cursor, Input& input, DockTree& tree) {
			auto& node = tree.get_node(drag_target->id);

			node.position += input.mouse.position - track_cursor->origin;
		});

	world.system<DockNodeRef, DockTree>("floating window drag")
		.with<TrackDrag>()
		.with<DragTarget>("$drag_target")
		.term_at(0).src("$drag_target")
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, DockNodeRef& dock_node, DockTree& tree) {
			tree.undock_window(it.get_var("drag_target"), dock_node.id);
		});

	world.system<Node, GlobalTransform, DockNodeRef, DockTree>()
		.with<DockSpaceNode>()
		.kind(Phases::PostUpdate)
		.each([](Node& node, GlobalTransform& transform, DockNodeRef& ref, DockTree& tree) {
			auto& dock_node = tree.get_node(ref.id);

			dock_node.size = node.size;
			dock_node.position = glm::vec2(transform.translation);
		});

	world.system<DockTree>()
		.kind(Phases::PostUpdate)
		.each([](DockTree& tree) {
			tree.update();
		});

	world.system<Node, Transform, DockNodeRef, DockTree>()
		.without<DockSpaceNode>()
		.kind(Phases::PostUpdate)
		.each([](flecs::entity entity, Node& node, Transform& transform, DockNodeRef& ref, DockTree& tree) {
			const auto& dock_node = tree.get_node(ref.id);

			transform.translation.x = dock_node.position.x;
			transform.translation.y = dock_node.position.y;

			node.sizing_policy.first = Node::Fixed{ dock_node.size.x };
			node.sizing_policy.second = Node::Fixed{ dock_node.size.y };
		});

	world.system<DockTree>()
		.kind(Phases::PostUpdate)
		.each([](DockTree& tree) {
			tree.cleanup();
		});

	world.add<DockTree>();
}

flecs::entity ps::create_dockspace(flecs::world &world, flecs::entity window, const std::string &name) {
	auto tree = world.get_ref<DockTree>();
	auto content_entity = window.target<WindowContent>(0);

	auto dockspace_entity = world.entity(content_entity, name.c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.add<DockSpaceNode>()
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.display = false,
			.absolute = true,
		});

	window.add<DockingSpace>(dockspace_entity);

	tree->add_dockspace(dockspace_entity);

	return dockspace_entity;
}

static std::uint64_t dock_preview_id = 0;

flecs::entity ps::create_dockspace_preview(flecs::world& world, glm::vec2 size, SplitAxis split_axis, DockSide dock_side) {
	const auto background_color = Color::from_uint(69, 133, 136, 127);
	const auto name = std::format("dockspace_preview_{}", dock_preview_id++);

	auto main_entity = world.entity(name.c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.add<DockPreviewNode>()
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.grow_direction = split_axis == SplitAxis::Horizontal ? Node::GrowDirection::Vertical : Node::GrowDirection::Horizontal,
		});

	auto top_left_entity = world.entity(main_entity, std::format("{}_top_left", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.grow_direction = split_axis == SplitAxis::Horizontal ? Node::GrowDirection::Vertical : Node::GrowDirection::Horizontal,
		});

	auto bot_right_entity = world.entity(main_entity, std::format("{}_bot_right", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.child_alignment = { 1.f, 1.f },
			.grow_direction = split_axis == SplitAxis::Horizontal ? Node::GrowDirection::Vertical : Node::GrowDirection::Horizontal,
		});

	auto preview_entity = world.entity(dock_side == DockSide::TopLeft ? top_left_entity : bot_right_entity, std::format("{}_preview", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = {
				split_axis == SplitAxis::Horizontal
					? std::pair<Node::SizePolicy, Node::SizePolicy>(Node::Grow{}, Node::Grow{ .max = size.y })
					: std::pair<Node::SizePolicy, Node::SizePolicy>(Node::Grow{ .max = size.x }, Node::Grow{})
			},
		});

	return main_entity;
}

flecs::entity ps::create_dockspace_inner_options(flecs::world& world, const DockNode& dock_node) {
	const auto background_color = Color::from_uint(69, 133, 136, 180);
	const auto border_radius = 4.f;

	const auto editor_root = world.lookup("editor_root");
	const auto name = std::format("dockspace_options_{}", dock_preview_id++);
	const auto size = std::min({ dock_node.size.x, dock_node.size.y, 150.f });

	DockingTarget docking_target = [&]() -> DockingTarget {
		if (dock_node.id != 0) {
			return NodeTarget{
				.id = dock_node.id
			};
		}

		return WindowTarget{
			.window = dock_node.windows[dock_node.active_window],
			.size = dock_node.size,
			.position = dock_node.position,
		};
	}();

	auto dockspace_entity = world.entity(editor_root, name.c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.add<DockOptionsNode>()
		.set<Transform>({
			.translation = glm::vec3(dock_node.position, 0.f)
		})
		.set<Node>({
			.sizing_policy = { Node::Fixed{ dock_node.size.x }, Node::Fixed{ dock_node.size.y } },
			.child_alignment = { 0.5f, 0.5f },
			.absolute = true,
		});

	auto preview_holder = world.entity(dockspace_entity, std::format("{}_preview_holder", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.absolute = true,
		});

	auto options_holder = world.entity(dockspace_entity, std::format("{}_options_holder", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Fixed{ size }, Node::Fixed{ size } },
			.child_gap = { 4.f, 4.f },
			.grow_direction = Node::GrowDirection::Vertical,
		});

	auto top_holder = world.entity(options_holder, std::format("{}_top_holder", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.child_gap = { 4.f, 4.f },
		});

	auto middle_holder = world.entity(options_holder, std::format("{}_middle_holder", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.child_gap = { 4.f, 4.f },
		});

	auto bottom_holder = world.entity(options_holder, std::format("{}_bottom_holder", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.child_gap = { 4.f, 4.f },
		});

	auto tl_pusher = world.entity(top_holder, std::format("{}_tl_pusher", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
		});

	auto top_docking_entity = world.entity(top_holder, std::format("{}_top_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::Horizontal,
			.dock_side = DockSide::TopLeft,
			.dock_options_container = dockspace_entity,
		});

		auto tr_pusher = world.entity(top_holder, std::format("{}_tr_pusher", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
		});

	auto left_docking_entity = world.entity(middle_holder, std::format("{}_left_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::Vertical,
			.dock_side = DockSide::TopLeft,
			.dock_options_container = dockspace_entity,
		});

	auto mid_docking_entity = world.entity(middle_holder, std::format("{}_mid_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::None,
			.dock_side = DockSide::TopLeft,
			.dock_options_container = dockspace_entity,
		});

	auto right_docking_entity = world.entity(middle_holder, std::format("{}_right_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::Vertical,
			.dock_side = DockSide::BotRight,
			.dock_options_container = dockspace_entity,
		});


	auto bl_pusher = world.entity(bottom_holder, std::format("{}_bl_pusher", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
		});

	auto bot_docking_entity = world.entity(bottom_holder, std::format("{}_bot_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::Horizontal,
			.dock_side = DockSide::BotRight,
			.dock_options_container = dockspace_entity,
		});

		auto br_pusher = world.entity(bottom_holder, std::format("{}_br_pusher", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
		});

	return dockspace_entity;
}

flecs::entity ps::create_dockspace_outer_options(flecs::world& world, const DockNode& dock_node) {
	const auto background_color = Color::from_uint(69, 133, 136, 180);
	const auto border_radius = 4.f;

	const auto editor_root = world.lookup("editor_root");
	const auto name = std::format("dockspace_options_{}", dock_preview_id++);

	DockingTarget docking_target = [&]() -> DockingTarget {
		if (dock_node.id != 0) {
			return NodeTarget{
				.id = dock_node.id
			};
		}

		return WindowTarget{
			.window = dock_node.windows[dock_node.active_window],
			.size = dock_node.size,
			.position = dock_node.position,
		};
	}();

	auto dockspace_entity = world.entity(editor_root, name.c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.add<DockOptionsNode>()
		.set<Transform>({
			.translation = glm::vec3(dock_node.position, 0.f)
		})
		.set<Node>({
			.sizing_policy = { Node::Fixed{ dock_node.size.x }, Node::Fixed{ dock_node.size.y } },
			.absolute = true,
		});

	auto preview_holder = world.entity(dockspace_entity, std::format("{}_preview_holder", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.absolute = true,
		});

	auto left_docking_entity = world.entity(dockspace_entity, std::format("{}_left_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Fixed{ 50.f }, Node::Fixed{ 100.f } },
			.self_alignment = { 0.f, 0.5f },
			.absolute = true,
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::Vertical,
			.dock_side = DockSide::TopLeft,
			.dock_options_container = dockspace_entity,
		});

	auto right_docking_entity = world.entity(dockspace_entity, std::format("{}_right_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Fixed{ 50.f }, Node::Fixed{ 100.f } },
			.self_alignment = { 1.f, 0.5f },
			.absolute = true,
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::Vertical,
			.dock_side = DockSide::BotRight,
			.dock_options_container = dockspace_entity,
		});

	auto top_docking_entity = world.entity(dockspace_entity, std::format("{}_top_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Fixed{ 100.f }, Node::Fixed{ 50.f } },
			.self_alignment = { 0.5f, 0.f },
			.absolute = true,
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::Horizontal,
			.dock_side = DockSide::TopLeft,
			.dock_options_container = dockspace_entity,
		});

	auto bot_docking_entity = world.entity(dockspace_entity, std::format("{}_bot_docking", name).c_str())
		.set<BackgroundColor>(background_color)
		.set<Node>({
			.sizing_policy = { Node::Fixed{ 100.f }, Node::Fixed{ 50.f } },
			.self_alignment = { 0.5f, 1.f },
			.absolute = true,
			.border_radius = border_radius,
		})
		.add<DockingOption>()
		.add<Button>()
		.set<DockData>(DockData{
			.target = docking_target,
			.split_axis = SplitAxis::Horizontal,
			.dock_side = DockSide::BotRight,
			.dock_options_container = dockspace_entity,
		});

	return dockspace_entity;
}
