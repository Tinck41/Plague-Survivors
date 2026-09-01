#include "immediate.h"

#include "ecsModule/textModule/module.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "gtx/easing.hpp"
#include "spdlog/spdlog.h"
#include "utils/visit.h"

#include <algorithm>
#include <format>
#include <queue>
#include <ranges>

using namespace se::editor;

constexpr auto initial_window_size = glm::vec2{ 100.f, 100.f };

constexpr float DEFAULT_WIDGET_LAYER     = 0.0f;
constexpr float FOCUSED_WIDGET_LAYER     = 0.5f;
constexpr float PREVIEW_WIDGET_LAYER     = 1.0f;
constexpr float DOCK_OPTION_WIDGET_LAYER = 2.0f;

// TODO:
// [ ] PushID()/PopID()
// [ ] PushStyle()/PopStyle()
// [ ] PushLayer()/PopLayer() ????
// [ ] Docking
//  - [x] change aspect ratio
//  - [x] multitab window
//  - [x] undock with following dock
//  - [x] animation for dock nodes appearance
//  - [x] resize dock options according to window size
//  - [x] preview with dock gap
//  - [x] foucs window when select tab, cant' be done immediately because selected window widget doesn't exist
//  - [x] fix bug when dock into center already docked window to another already docked window 
//  - [x] fix bug when dock to dockspace two docked window [  |  ]
//  - [x] dock_inner_options - fix padding and create fake widgets
// [ ] remove drag/drop_id
// [ ] think about remove all direct acces to entity component
// [ ] better focus order handling for both window and dock nodes

void Immediate::init(flecs::world& world) {
	ctx.world = &world;
	ctx.root = world.entity("immediate_root").add(flecs::OrderedChildren);
	ctx.update_query = world.query_builder<Immediate, ImmediateId, Node, SizeStrategy, Transform, GlobalTransform>().with<GrowDirection>().term_at(0).inout().build();
	ctx.check_query = world.query_builder<Immediate, ImmediateId, Node>().term_at(0).inout().build();
	ctx.cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	ctx.cursor_type = SDL_SYSTEM_CURSOR_DEFAULT;
	ctx.last_cursor_type = SDL_SYSTEM_CURSOR_DEFAULT;

	SDL_SetCursor(ctx.cursor);
}

void Immediate::begin(std::string name, WindowFlags flags, std::optional<glm::vec2> pos, std::optional<glm::vec2> size) {
	const auto id = hash(name);

	if (!ctx.id_to_window.contains(id)) {
		const auto focus_order = ctx.id_to_window.size();

		ctx.id_to_window[id] = {
			.id = id,
			.focus_order = focus_order,
			.name = name,
			.size = initial_window_size,
			.flags = flags,
		};

		ctx.windows_focus_order.push_back(&ctx.id_to_window.at(id));
	}

	auto& window = ctx.id_to_window.at(id);

	ctx.current_window = &window;

	if (window.disabled) {
		return;
	}

	if (window.dock_node_id) {
		const auto dock_node = get_dock_node(window.dock_node_id.value());

		assert(dock_node);

		const auto [start_pos, start_size] = [&] {
			if (dock_node->parent_id) {
				const auto parent = get_dock_node(dock_node->parent_id);
				const auto side_coef = parent->children[0] == dock_node->id ? 0.f : 1.f;

				const auto pos = parent->split_axis == SplitAxis_Horizontal
					? glm::vec2{ dock_node->position.x, dock_node->position.y + (parent->size.y - ctx.dock_ctx.node_gap) * side_coef - dock_node->size.y * side_coef }
					: glm::vec2{ parent->position.x + parent->size.x * side_coef, dock_node->position.y };
				const auto size = parent->split_axis == SplitAxis_Horizontal
					? glm::vec2{ dock_node->size.x, 0.f }
					: glm::vec2{ 0.f, dock_node->size.y };

				return std::pair{ pos, size };
			}

			return std::pair{ glm::vec2(0.f), dock_node->size };
		}();

		window.pos = animation_value(std::format("{}_pos", name), start_pos, dock_node->position, ctx.anim_pop_rate, dock_node->skip_next_anims);
		window.size = animation_value(std::format("{}_size", name), start_size, dock_node->size, ctx.anim_pop_rate, dock_node->skip_next_anims);

		dock_node->skip_next_anims = false;
	}

	auto window_widget = create_widget(name, WidgetFlags_Clickable);

	push_parent(window_widget);

	const auto no_titlebar = static_cast<bool>(flags & WindowFlags_NoTitlebar);
	const auto no_move = static_cast<bool>(flags & WindowFlags_NoMove);
	const auto no_resize = static_cast<bool>(flags & WindowFlags_NoResize);

	if (is_new(*window_widget)) {
		window_widget->entity
			.set<BackgroundColor>(Color::from_hex("#282828"))
			.set<BorderColor>(Color::from_hex("#504945"))
			.set(GrowDirection::Vertical)
			.set<Node>({
				.padding = { 4.f, 4.f, 4.f, 4.f },
				.absolute = true,
				.border_radius = 4.f,
				.border_width = 1.f,
			});

		if (!window.dock_node_id) {
			if (static_cast<bool>(flags & WindowFlags_FitViewport)) {
				window.size.x = static_cast<float>(ctx.viewport_size.x);
				window.size.y = static_cast<float>(ctx.viewport_size.y);

				window.main_window = true;
			}
			else if (size) {

				window.size = size.value();
			}
			else {
				window.size = initial_window_size;
			}

			if (pos) {
				window.pos = pos.value();
			}
		}
	}

	auto& border_color = window_widget->entity.ensure<BorderColor>();

	if (window.has_focus) {
		border_color = Color::from_hex("#d65d0e");
	}
	else {
		border_color = Color::from_hex("#504945");
	}

	if (static_cast<bool>(flags & WindowFlags_FitViewport)) {
		const auto main_window = ctx.world->get<WindowModule>().main_window;

		int width;
		int height;

		SDL_GetWindowSize(main_window, &width, &height);

		window.size.x = static_cast<float>(width);
		window.size.y = static_cast<float>(height);
	}

	if (!no_move && drag_interaction(window_widget)) {
		const auto delta = ctx.mouse_input.position - last_ctx.mouse_input.position;

		if (window.dock_node_id) {
			const auto root = get_dock_root_node(window.dock_node_id.value());

			if (root->host_window != 0) {
				auto& host_window = ctx.id_to_window.at(root->host_window);

				if (!static_cast<bool>(host_window.flags & WindowFlags_NoMove)) {
					root->position += delta;
				}
			}
			else {
			  root->position += delta;
			}

			dock_node_skip_anims(root->id);
		}
		else {
			window.pos += delta;
		}

		focus_window(window);
	}

	if (!no_titlebar) {
		if (window.dock_node_id) {
			tabsbar(window);
		}
		else {
			titlebar(window);
		}
	}

	if (!no_resize && !window.dock_node_id && !window.colapsed) {
		auto resize_button_widget = create_widget(std::format("{}_resize_button", name), WidgetFlags_Clickable);

		if (is_new(*resize_button_widget)) {
			resize_button_widget->entity
				.set<BackgroundColor>(RED)
				.set(GrowDirection::Horizontal)
				.set<SizeStrategy>({
					.x = Fixed{ 10.f },
					.y = Fixed{ 10.f },
				})
				.set<Node>({
					.self_alignment = { 1.f, 1.f },
					.absolute = true,
				});
		}

		if (drag_interaction(resize_button_widget)) {
			window.size += ctx.mouse_input.position - last_ctx.mouse_input.position;
		}
	}

	if (!window.colapsed) {
		auto content_widget = create_widget(std::format("{}_content", name));

		push_parent(content_widget);

		if (is_new(*content_widget)) {
			content_widget->entity
				.set(GrowDirection::Horizontal)
				.set<SizeStrategy>({
						.x = Grow{},
						.y = Grow{},
				})
				.set<Node>({
					.overflow = { Node::Overflow::Clip, Node::Overflow::Clip },
				});
		}
	}

	push_parent(window_widget);

	const auto overlay_widget = create_widget(std::format("{}_overlay", name));

	if (is_new(*overlay_widget)) {
		overlay_widget->entity
			.set(GrowDirection::Horizontal)
			.set<SizeStrategy>({
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.absolute = true,
			});

		window.overlay_id = overlay_widget->id;
	}

	pop_parent();
}

void Immediate::end() {
	const auto window = ctx.current_window;

	if (!window->disabled) {
		if (!window->colapsed) {
			pop_parent(); // content entity

			if (!window->dock_node_id && !static_cast<bool>(window->flags & WindowFlags_NoMove)) {
				grip_window_borders(*window);
			}
		}

		pop_parent(); // window entity
	}

	ctx.current_window = nullptr;
}

void Immediate::dockspace(const std::string& name, glm::vec2 size) {
	const auto id = hash(name);
	auto& active_window = *ctx.current_window;

	if (active_window.disabled || active_window.colapsed) {
		return;
	}

	const auto dockspace_widget = create_widget(name, WidgetFlags_Clickable);

	if (is_new(*dockspace_widget)) {
		dockspace_widget->entity
			.set<ImmediateId>({ dockspace_widget->id })
			.set<BackgroundColor>(Color::from_hex("#282828"))
			.set<BorderColor>(Color::from_hex("#504945"))
			.set(GrowDirection::Horizontal)
			.set<Node>({
				.border_radius = 4.f,
				.border_width = 1.f,
			});

		if (size != glm::vec2(0.f, 0.f)) {
			dockspace_widget->entity.set<SizeStrategy>({
				.x = Fixed{ size.x },
				.y = Fixed{ size.y },
			});
		}
		else {
			dockspace_widget->entity.set<SizeStrategy>({
				.x = Grow{},
				.y = Grow{},
			});
		}

		const auto dock_node = create_dock_root(id, active_window.pos, active_window.size);

		dock_node->dockspace = true;
		dock_node->central_node = true;
		dock_node->central_node_id = dock_node->id;
		dock_node->host_window = active_window.id;

		active_window.dock_node_host_id = id;
	}

	const auto dock_node = get_dock_node(id);
	const auto& pos = dockspace_widget->entity.ensure<GlobalTransform>().translation;
	const auto& node_size = dockspace_widget->entity.ensure<Node>().size;

	dock_node->position = glm::vec2(pos);
	dock_node->size = node_size;
}

bool Immediate::button(std::string name) {
	const auto& active_window = *ctx.current_window;

	if (active_window.disabled || active_window.colapsed) {
		return false;
	}

	auto button_widget = create_widget(name, WidgetFlags_Clickable);

	if (is_new(*button_widget)) {
		button_widget->entity
			.set(GrowDirection::Horizontal)
			.set<BackgroundColor>(RED)
			.set<SizeStrategy>({
				.x = Fixed{ 10.f },
				.y = Fixed{ 10.f },
			})
			.set<Text>(std::string(name))
			.set<TextFont>({
				.handle = ctx.world->get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
				.size = 24,
			})
			.set<TextColor>(WHITE)
			.add<Node>();
	}

	return button_interaction(button_widget);
}

void Immediate::text(std::string text) {
	const auto& active_window = *ctx.current_window;

	if (active_window.disabled || active_window.colapsed) {
		return;
	}

	auto text_widget = create_widget(text);

	if (is_new(*text_widget)) {
		text_widget->entity
			.set<SizeStrategy>({
				.x = Fixed{ 10.f },
				.y = Fixed{ 10.f },
			})
			.set(GrowDirection::Horizontal)
			.set<Text>(std::string(text))
			.set<TextFont>({
				.handle = ctx.world->get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
				.size = 24,
			})
			.set<TextColor>(WHITE)
			.add<Node>();
	}
}

void Immediate::begin_frame() {
	const auto& input = ctx.world->get<Input>();
	const auto main_window = ctx.world->get_ref<WindowModule>()->main_window;

	ctx.mouse_input = input.mouse;
	ctx.general_overlay_widget = create_widget("overlay_widget"); // empty holder widget

	SDL_GetWindowSize(main_window, &ctx.viewport_size.x, &ctx.viewport_size.y);

	if (ctx.mouse_input.left.pressed) {
		ctx.drag_drop_ctx.start_pos = ctx.mouse_input.position;
	}

	process_drag_drop();
	update_dock_nodes();
}

void Immediate::end_frame() {
	for (const auto root_id : ctx.dock_ctx.roots) {
		const auto root = get_dock_node(root_id);

		push_parent(nullptr);

		const auto root_overlay_widget = create_widget(std::format("dock_{}_widget_overlay", root_id));

		root->overlay_widget_id = root_overlay_widget->id;

		if (is_new(*root_overlay_widget)) {
			root_overlay_widget->entity
				.set<Node>({
					.absolute = true,
				})
				.set(GrowDirection::Horizontal);
		}

		auto& transform = root_overlay_widget->entity.ensure<Transform>();
		auto& size = root_overlay_widget->entity.ensure<SizeStrategy>();

		transform.translation = glm::vec3(root->position, 0.f);
		size.x = Fixed{ root->size.x };
		size.y = Fixed{ root->size.y };

		grip_dock_borders(*root);

		pop_parent();
	}

	assert(ctx.parent_stack.empty());

	if (clear_on_frame_end) {
		clear_inactive_widgets();
		clear_inactive_animations();
	}

	auto render_queue = compose_render_queue();

	ctx.hot_widget = nullptr;

	const auto total_dfs_indices = calculate_dfs_indices(render_queue);
	const auto total_bfs_indices = calculate_bfs_indices(render_queue);

	assert(total_bfs_indices == total_dfs_indices);
	assert(total_bfs_indices == (ctx.widgets_num - 1)); // general_overlay_widget doesn't count

	if (ctx.cursor_type != ctx.last_cursor_type) {
		SDL_DestroyCursor(ctx.cursor);

		ctx.cursor = SDL_CreateSystemCursor(ctx.cursor_type);

		SDL_SetCursor(ctx.cursor);
	}
	ctx.last_cursor_type = ctx.cursor_type;
	ctx.cursor_type = SDL_SYSTEM_CURSOR_DEFAULT;

	size_t check_sum = 0;

	ctx.update_query.run([&check_sum](flecs::iter& it) {
		while (it.next()) {
			auto& im = it.field<Immediate>(0)[0];

			const auto id_field = it.field<ImmediateId>(1);
			const auto node_field = it.field<Node>(2);
			const auto size_field = it.field<SizeStrategy>(3);
			const auto transform_field = it.field<Transform>(4);
			const auto global_transform_field = it.field<GlobalTransform>(5);

			for (auto i : it) {
				const auto& id = id_field[i].value;
				auto& node = node_field[i];

				++check_sum;

				if (!im.ctx.id_to_widget.contains(id)) {
					//node.display = false;

					continue;
				}

				auto& size = size_field[i];
				auto& transform = transform_field[i];
				auto& global_transform = global_transform_field[i];
				auto& widget = im.ctx.id_to_widget.at(id);

				widget.last_rect = glm::vec4(glm::vec2(global_transform.translation), glm::vec2(global_transform.translation) + node.size);

				if (im.ctx.id_to_window.contains(id)) {
					const auto& window = im.ctx.id_to_window[id];

					if (window.colapsed) {
						size.y = Fit{};
					}
					else {
						size.x = Fixed{ window.size.x };
						size.y = Fixed{ window.size.y };
					}

					transform.translation = glm::vec3(window.pos, 0.f);
				}
			}
		}
	});

	assert(total_dfs_indices == check_sum );

	ctx.check_query.run([](flecs::iter& it) {
		while (it.next()) {
			auto& im = it.field<Immediate>(0)[0];

			const auto id_field = it.field<ImmediateId>(1);
			const auto node_field = it.field<Node>(2);

			for (auto i : it) {
				const auto entity = it.entity(i);
				const auto& id = id_field[i].value;
				auto& node = node_field[i];

				assert(entity.has<GrowDirection>());
				assert(entity.has<SizeStrategy>());
				assert(entity.has<NodeIndex>());
				assert(im.ctx.id_to_widget.contains(id));
			}
		}
	});

	last_ctx = ctx;
	ctx.roots.clear();

	ctx.current_window = nullptr;
	ctx.widgets_num = 0;
	//ctx.dock_ctx.payload.docked_target.reset();

	++ctx.frame_count;
}

float Immediate::animation_value(std::string name, float initial, float target, float rate, bool skip) {
	const auto key = hash(name);

	if (!ctx.id_to_anim.contains(key)) {
		ctx.id_to_anim[key] = {
			.first_active_frame = ctx.frame_count,
			.initial = initial,
			.current = initial,
		};
	}

	auto& animation = ctx.id_to_anim[key];

	animation.target = target;
	animation.rate = rate,
	animation.last_active_frame = ctx.frame_count;
	animation.current += (animation.target - animation.current) * (1.f - std::pow(2.f, -rate * ctx.world->delta_time()));

	if (skip) {
		animation.current = animation.target;
	}

	return animation.current;
}

glm::vec2 Immediate::animation_value(std::string name, glm::vec2 initial, glm::vec2 target, float rate, bool skip) {
	return {
		animation_value(name + "_0", initial.x, target.x, rate, skip),
		animation_value(name + "_1", initial.y, target.y, rate, skip)
	};
}

// TODO
float Immediate::animation_precent(std::string name, float initial, float target, float rate) {
	const auto key = hash(name);

	if (!ctx.id_to_anim.contains(key)) {
		ctx.id_to_anim[key] = {
			.first_active_frame = ctx.frame_count,
			.initial = initial,
			.current = initial,
		};
	}

	auto& animation = ctx.id_to_anim[key];

	animation.target = target;
	animation.rate = rate,
	animation.last_active_frame = ctx.frame_count;
	animation.current += (animation.target - animation.current) * (1.f - std::pow(2.f, -rate * ctx.world->delta_time()));

	return animation.current;
}

std::uint64_t Immediate::hash(std::string_view name) {
	return std::hash<std::string>{}(name.data());
}

Immediate::Widget* Immediate::get_widget_by_id(std::uint64_t id) {
	if (!ctx.id_to_widget.contains(id)) {
		return nullptr;
	}

	return &ctx.id_to_widget.at(id);
}

Immediate::Widget* Immediate::create_widget(std::string_view name, WidgetFlags flags) {
	const auto id = hash(name);
	auto widget = get_widget_by_id(id);

	if (widget == nullptr) {
		ctx.id_to_widget[id] = {
			.entity             = ctx.world->entity(ctx.root, name.data()).set<ImmediateId>({ .value = id }),
			.id                 = id,
			.first_active_frame = ctx.frame_count,
			.flags              = flags,
		};

		widget = &ctx.id_to_widget.at(id);
	}

	widget->first  = nullptr;
	widget->last   = nullptr;
	widget->prev   = nullptr;
	widget->next   = nullptr;
	widget->parent = nullptr;

	insert_widget_in_tree(widget, get_parent());

	widget->last_active_frame = ctx.frame_count;

	++ctx.widgets_num;

	return widget;
}

void Immediate::insert_widget_in_tree(Widget* widget, Widget* parent) {
	if (parent != nullptr) {
		if (parent->first == nullptr) {
			parent->first = widget;
			parent->last = widget;
		}
		else {
			parent->last->next = widget;
			widget->prev = parent->last;
			parent->last = widget;
		}

		widget->parent = parent;

		widget->entity.child_of(parent->entity);
	}
	else {
		widget->entity.child_of(ctx.root);
	}
}

void Immediate::children(Widget* parent, const std::function<void(Widget*)>& callback) {
	auto child = parent->first;

	while (child != nullptr) {
		callback(child);

		child = child->next;
	}
}

void Immediate::dfs(Widget* root, const std::function<void(Widget*)>& callback) {
	callback(root);

	children(root, [&](Widget* cild) {
		dfs(cild, callback);
	});
}

void Immediate::bfs(std::span<Widget*> roots, const std::function<void(Widget*)>& callback) {
	std::queue<Widget*> queue;

	for (const auto& root : roots) {
		queue.push(root);
	}

	while (!queue.empty()) {
		auto widget = queue.front();
		queue.pop();

		callback(widget);

		children(widget, [&queue](Widget* child) {
			queue.push(child);
		});
	}
}

std::vector<Immediate::HashId> Immediate::compose_render_queue() {
	const auto& dock_roots = ctx.dock_ctx.roots;
	const auto& windows = ctx.windows_focus_order;

	std::vector<WidgetGroup> render_groups;

	for (const auto window : windows) {
		if (window->dock_node_id || window->disabled) {
			continue;
		}

		auto group = WidgetGroup{
			.widgets = { window->id },
			.sort_value = window->focus_order,
		};

		if (window->dock_node_host_id) {
			const auto node = get_dock_node(window->dock_node_host_id.value());

			bfs(node->id, [&](DockNode* node) {
				if (node->active_window == 0) {
					return;
				}

				const auto& docked_window = ctx.id_to_window.at(node->active_window);

				group.widgets.push_back(docked_window.id);

				if (!window->main_window) {
					group.sort_value = std::max(docked_window.focus_order, docked_window.focus_order);
				}
			});

			group.widgets.push_back(node->overlay_widget_id);
		}

		render_groups.push_back(group);
	}

	for (const auto root_id : dock_roots) {
		const auto root = get_dock_node(root_id);

		// Dockspaces handled through the windows
		if (root->dockspace) {
			continue;
		}

		std::vector<HashId> dock_widgets;
		size_t sort_value = 0;

		bfs(root_id, [&](DockNode* node) {
			if (node->active_window == 0) {
				return;
			}

			const auto& window = ctx.id_to_window.at(node->active_window);

			dock_widgets.push_back(window.id);

			sort_value = std::max(window.focus_order, sort_value);
		});

		dock_widgets.push_back(root->overlay_widget_id);

		render_groups.push_back({
			.widgets = dock_widgets,
			.sort_value = sort_value,
		});
	}

	std::ranges::sort(render_groups, {}, &WidgetGroup::sort_value);

	std::vector<HashId> result;

	for (const auto& group : render_groups) {
		result.append_range(group.widgets);
	}

	children(ctx.general_overlay_widget, [&](Widget* child) {
		result.push_back(child->id);
	});

	return result;
}

size_t Immediate::calculate_dfs_indices(const std::vector<HashId>& render_queue) {
	if (render_queue.empty()) {
		return 0;
	}

	size_t dfs_index = 0;

	std::unordered_set<flecs::entity_t> proccesed;

	for (const auto widget_id : render_queue) {
		dfs(&ctx.id_to_widget.at(widget_id), [&](Widget* widget) {
			if (!widget) {
				return;
			}

			const auto name = widget->entity.name();
			assert(!proccesed.contains(widget->entity));

			proccesed.emplace(widget->entity);

			auto& index = widget->entity.ensure<NodeIndex>();

			index.dfs = dfs_index++;
			index.external_dfs_source = true;

			if (widget->flags & WidgetFlags_Clickable) {
				const auto& mouse_input = ctx.mouse_input;

				if (mouse_input.position.x > widget->last_rect.x && mouse_input.position.x < widget->last_rect.z &&
					mouse_input.position.y > widget->last_rect.y && mouse_input.position.y < widget->last_rect.w
				) {
					ctx.hot_widget = widget;
				}
			}
		});
	}

	return dfs_index;
}

size_t Immediate::calculate_bfs_indices(const std::vector<HashId>& render_queue) {
	if (render_queue.empty()) {
		return 0;
	}

	std::vector<Widget*> widgets;

	widgets.reserve(render_queue.size());

	for (const auto widget_id : render_queue) {
		widgets.push_back(&ctx.id_to_widget.at(widget_id));
	}

	size_t bfs_index = 0;

	bfs(widgets, [&](Widget* widget) {
		if (!widget) {
			return;
		}

		auto& index = widget->entity.ensure<NodeIndex>();

		index.bfs = bfs_index++;
		index.external_bfs_source = true;
	});

	return bfs_index;
}

void Immediate::clear_inactive_widgets() {
	std::vector<Widget*> widgets_to_delete;

	for (auto& [_, widget] : ctx.id_to_widget) {
		if (widget.last_active_frame < ctx.frame_count) {
			widgets_to_delete.push_back(&widget);
		}
	}

	for (const auto& widget : widgets_to_delete) {
		widget->entity.destruct();
		ctx.id_to_widget.erase(widget->id);
	}
}

void Immediate::clear_inactive_animations() {
	std::vector<HashId> animations_to_delete;

	for (auto& [id, widget] : ctx.id_to_anim) {
		if (widget.last_active_frame < ctx.frame_count) {
			animations_to_delete.push_back(id);
		}
	}

	for (const auto id : animations_to_delete) {
		ctx.id_to_anim.erase(id);
	}
}

void Immediate::process_drag_drop() {
	if (ctx.drag_drop_ctx.state == DragAndDropState_None || !ctx.dock_ctx.payload.docked_target) {
		return;
	}

	const auto& mouse_input = ctx.mouse_input;
	auto& payload = ctx.dock_ctx.payload;

	std::optional<DockTarget> target;

	for (const auto& window : ctx.windows_focus_order | std::ranges::views::reverse) {
		if (window->disabled || window->colapsed) {
			continue;
		}

		const auto skip = se::visit(payload.docked_target.value(), se::visitors{
			[&](Window* window_target) {
				return window_target->id == window->id;
			},
			[&](DockNode* node) {
				return window->dock_node_id ? get_dock_root_node(window->dock_node_id.value())->id == node->id : false;
			}
		});

		if (skip) {
			continue;
		}

		if (mouse_input.position.x < window->pos.x || mouse_input.position.x > window->pos.x + window->size.x ||
			mouse_input.position.y < window->pos.y || mouse_input.position.y > window->pos.y + window->size.y
		) {
			continue;
		}

		if (window->dock_node_id) {
			target = get_dock_node(window->dock_node_id.value());
		}
		else if (window->dock_node_host_id) {
			const auto dock_node = get_dock_node(window->dock_node_host_id.value());

			if (auto central_node = get_dock_node(dock_node->central_node_id)) {
				target = central_node;
			}
			else {
				target = dock_node;
			}
		}
		else if (!static_cast<bool>(window->flags & WindowFlags_NoDocking)) {
			target = window;
		}

		break;
	}

	if (target && std::holds_alternative<DockNode*>(target.value())) {
		const auto node = std::get<DockNode*>(target.value());
		const auto root = get_dock_root_node(node->id);

		bfs(root->id, [&](DockNode* node) {
			if (mouse_input.position.x < node->position.x || mouse_input.position.x > node->position.x + node->size.x ||
				mouse_input.position.y < node->position.y || mouse_input.position.y > node->position.y + node->size.y
			) {
				return;
			}

			target = node;
		});
	}

	if (ctx.drag_drop_ctx.state == DragAndDropState_Dragging && target) {
		payload.dock_side = DockSide_None;
		payload.split_axis = SplitAxis_None;

		se::visit(target.value(), se::visitors{
			[&](Window* window) {
				dock_inner_options(window);
			},
			[&](DockNode* node) {
				auto root_node = get_dock_root_node(node->id);

				if (node != root_node) {
					dock_inner_options(node);
				}
				dock_outer_options(root_node->id);
			}
		});

		if (payload.split_axis != SplitAxis_None || payload.dock_side != DockSide_None) {
			dock_preview(payload.docking_target.value(), payload.docked_target.value(), payload.split_axis, payload.dock_side);
		}
	}
	else if (ctx.drag_drop_ctx.state == DragAndDropState_Dropping) {
		if (ctx.dock_ctx.payload.split_axis != SplitAxis_None || ctx.dock_ctx.payload.dock_side != DockSide_None) {
			se::visit(payload.docked_target.value(), se::visitors{
				[&](Window* window) {
					if (window->dock_node_id) {
						undock_window(*window);
					}
				},
				[](DockNode* node) {
					// TODO
				}
			});
			apply_dock(payload.docking_target.value(), payload.docked_target.value(), ctx.dock_ctx.payload.split_axis, ctx.dock_ctx.payload.dock_side);
		}
		else if (ctx.dock_ctx.payload.docked_target) {
			se::visit(ctx.dock_ctx.payload.docked_target.value(), se::visitors{
				[&](Window* window) {
					if (mouse_input.position.x > window->pos.x && mouse_input.position.x < window->pos.x + window->size.x &&
						mouse_input.position.y > window->pos.y && mouse_input.position.y < window->pos.y + window->size.y
					) {
						return;
					}

					undock_window(*window);
					focus_window(*window);

					window->pos = ctx.mouse_input.position;
				},
				[](DockNode* node) {

				}
			});
		}

		payload.dock_side = DockSide_None;
		payload.split_axis = SplitAxis_None;
		payload.docked_target.reset();
		payload.docking_target.reset();

		ctx.drag_drop_ctx.state = DragAndDropState_None;
	}

	if (mouse_input.left.released) {
		ctx.drag_drop_ctx.state = DragAndDropState_Dropping;
	}
}

void Immediate::push_parent(std::uint64_t widget_id) {
	ctx.parent_stack.push(get_widget_by_id(widget_id));
}

void Immediate::push_parent(Widget* widget) {
	ctx.parent_stack.push(widget);
}

void Immediate::pop_parent() {
	ctx.parent_stack.pop();
}

Immediate::Widget* Immediate::get_parent() {
	if (ctx.parent_stack.empty()) {
		return nullptr;
	}

	return ctx.parent_stack.top();
}

bool Immediate::is_widget_hovered(const Widget& widget) {
	if (widget.id == 0) {
		return false;
	}

	const auto& mouse_input = ctx.mouse_input;

	return mouse_input.position.x > widget.last_rect.x & mouse_input.position.x < widget.last_rect.z &&
		mouse_input.position.y > widget.last_rect.y & mouse_input.position.y < widget.last_rect.w;
}

bool Immediate::drag_interaction(Widget* widget) {
	bool result = false;

	if (ctx.active_widget == widget) {
		if (ctx.mouse_input.left.remain) {
			result = true;
		}
		else if (ctx.mouse_input.left.released) {
			result = false;

			ctx.active_widget = nullptr;
		}
	} else if (!ctx.active_widget && ctx.hot_widget == widget && ctx.mouse_input.left.pressed) {
		ctx.active_widget = widget;
	}

	return result;
}

bool Immediate::drag_offset_interaction(Widget* widget, glm::vec2 offset) {
	bool result = false;

	if (ctx.active_widget == widget) {
		if (ctx.mouse_input.left.remain) {
			const auto delta = glm::abs(ctx.drag_drop_ctx.start_pos - ctx.mouse_input.position);

			result = delta.x > offset.x || delta.y > offset.y;
		}
		else if (ctx.mouse_input.left.released) {
			result = false;

			ctx.active_widget = nullptr;
		}
	}
	else if (!ctx.active_widget && ctx.hot_widget == widget && ctx.mouse_input.left.pressed) {
		ctx.active_widget = widget;
	}

	return result;
}

bool Immediate::button_interaction(Widget* widget) {
	bool result = false;

	if (ctx.active_widget == widget && ctx.mouse_input.left.released && ctx.hot_widget == widget) {
		result = true;

		ctx.active_widget = nullptr;
	}
	else if (!ctx.active_widget && ctx.hot_widget == widget && ctx.mouse_input.left.pressed) {
		ctx.active_widget = widget;
	}

	return result;
}

bool Immediate::hover_interaction(Widget* widget) {
	return !ctx.active_widget && ctx.hot_widget == widget;
}

bool Immediate::is_collide_with(std::uint64_t left, std::uint64_t right) {
	auto left_widget_rect = get_widget_by_id(left)->last_rect;
	auto right_widget_rect = get_widget_by_id(right)->last_rect;

	return left_widget_rect.x > right_widget_rect.x && left_widget_rect.x < right_widget_rect.z &&
		left_widget_rect.y > right_widget_rect.y && left_widget_rect.w < right_widget_rect.w;
}

bool Immediate::is_mouse_collide_with(std::uint64_t widget_id) {
	const auto& mouse_pos = ctx.mouse_input.position;
	const auto& widget_rect = get_widget_by_id(widget_id)->last_rect;

	return mouse_pos.x > widget_rect.x && mouse_pos.x < widget_rect.z &&
		mouse_pos.y > widget_rect.y && mouse_pos.y < widget_rect.w;
}

bool Immediate::is_new(const Widget& widget) {
	return widget.first_active_frame == ctx.frame_count;
}

void Immediate::titlebar(Window& window) {
	const auto name = window.name;

	const auto no_move = static_cast<bool>(window.flags & WindowFlags_NoMove);
	const auto no_collasee = static_cast<bool>(window.flags & WindowFlags_NoCollasee);
	const auto no_close = static_cast<bool>(window.flags & WindowFlags_NoClose);

	auto titlebar_widget = create_widget(std::format("{}_titlebar", name), WidgetFlags_Clickable);

	push_parent(titlebar_widget);

	if (is_new(*titlebar_widget)) {
		titlebar_widget->entity
			.add<Node>()
			.set(GrowDirection::Horizontal)
			.set<SizeStrategy>({
				.x = Grow{},
				.y = Fixed{ 28.f },
			})
			.set<BackgroundColor>(Color::from_hex("#928374"));
	}

	if (!no_move && drag_interaction(titlebar_widget)) {
		window.pos += ctx.mouse_input.position - last_ctx.mouse_input.position;

		if (!window.colapsed) {
			ctx.drag_drop_ctx.state = DragAndDropState_Dragging;

			ctx.dock_ctx.payload.docked_target = &window;
		}

		focus_window(window);
	}

	if (!no_collasee) {
		auto collasee_button_widget = create_widget(std::format("{}_collasee_button", name), WidgetFlags_Clickable);

		if (is_new(*collasee_button_widget)) {
			collasee_button_widget->entity
				.add<Node>()
				.set(GrowDirection::Horizontal)
				.set<SizeStrategy>({
					.x = Fixed{ 28.f },
					.y = Fixed{ 28.f },
				})
				.set<BackgroundColor>(Color::from_hex("#282828"));
		}

		if (button_interaction(collasee_button_widget)) {
			window.colapsed = !window.colapsed;
		}
	}

	auto title_widget = create_widget(std::format("{}_title", name));

	if (is_new(*title_widget)) {
		title_widget->entity
			.set(GrowDirection::Horizontal)
			.set<Text>(std::string(name))
			.set<TextFont>({
				.handle = ctx.world->get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
				.size = 24,
			})
			.set<TextColor>(WHITE);
	}

	auto pusher_widget = create_widget(std::format("{}_pusher", name));

	if (is_new(*pusher_widget)) {
		pusher_widget->entity
			.set(GrowDirection::Horizontal)
			.set<SizeStrategy>({
				.x = Grow{},
				.y = Grow{},
			})
			.add<Node>();
	}

	if (!no_close) {
		auto close_button_widget = create_widget(std::format("{}_close_button", name), WidgetFlags_Clickable);

		if (is_new(*close_button_widget)) {
			close_button_widget->entity
				.add<Node>()
				.set(GrowDirection::Horizontal)
				.set<BackgroundColor>(RED)
				.set<SizeStrategy>({
					.x = Fixed{ 28.f },
					.y = Fixed{ 28.f },
				});
		}

		if (button_interaction(close_button_widget)) {
			// TODO: close window
		}
	}

	pop_parent();
}

void Immediate::tabsbar(Window& window) {
	if (!window.dock_node_id) {
		return;
	}

	const auto dock_node = get_dock_node(window.dock_node_id.value());
	if (!dock_node) {
		return;
	}

	const auto root_node = get_dock_root_node(dock_node->id);
	const auto tabsbar_widget = create_widget(std::format("{}_tabsbar", window.name), WidgetFlags_Clickable);
	const auto& window_ids = dock_node->windows;
	const auto active_window = dock_node->active_window;
	const auto& host_window = ctx.id_to_window.at(root_node->host_window);

	if (!(host_window.flags & WindowFlags_NoMove) && drag_interaction(tabsbar_widget)) {
		const auto root_node = get_dock_root_node(dock_node->id);

		root_node->position += ctx.mouse_input.position - last_ctx.mouse_input.position;

		ctx.drag_drop_ctx.state = DragAndDropState_Dragging;
		ctx.dock_ctx.payload.docked_target = root_node;

		focus_window(window);
		dock_node_skip_anims(root_node->id);
	}

	push_parent(tabsbar_widget);

	for (const auto window_id : window_ids) {
		const auto tab_window = &ctx.id_to_window.at(window_id);
		const auto tab_widget = create_widget(std::format("{}_tab", tab_window->name), WidgetFlags_Clickable);
		const auto is_active = window_id == active_window;

		push_parent(tab_widget);

		if (is_active) {
			const auto tab_highlight_widget = create_widget(std::format("{}_tab_highlight", tab_window->name));

			if (is_new(*tab_highlight_widget)) {
				tab_highlight_widget->entity
					.add<Node>()
					.set<ImmediateId>({ tab_highlight_widget->id })
					.set<BackgroundColor>(Color::from_hex("#d65d0e"))
					.set(GrowDirection::Horizontal)
					.set<SizeStrategy>({
						.x = Grow{},
						.y = Fixed{ 2.5f },
					});
			}

		}

		const auto tab_name_widget = create_widget(std::format("{}_tab_name", tab_window->name));

		if (is_new(*tab_widget)) {
			tab_widget->entity
				.add<Node>()
				.set<ImmediateId>({ tab_widget->id })
				.set(GrowDirection::Vertical)
				.set<SizeStrategy>({
					.x = Fixed{ 72.f }, // TODO
					.y = Fixed{ 24.f },
				});
		}

		if (is_new(*tab_name_widget)) {
			tab_name_widget->entity
				.add<Node>()
				.set(GrowDirection::Horizontal)
				.set<ImmediateId>({ tab_name_widget->id })
				.set<Text>(std::string(tab_window->name))
				.set<TextFont>({
					.handle = ctx.world->get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
					.size = 24,
				})
				.set<TextColor>(WHITE);
		}

		if (button_interaction(tab_widget)) {
			dock_node->next_active_window = window_id;

			dock_node_skip_anims(dock_node->id);
			focus_window(ctx.id_to_window.at(window_id));
		}

		tab_widget->entity.set<BackgroundColor>({ is_active ? Color::from_uint(40, 40, 40, 255) : Color::from_hex("#928374") });

		auto& tab_name_color = tab_name_widget->entity.ensure<TextColor>();

		if (drag_offset_interaction(tab_widget, glm::vec2(10.f, 10.f))) {
			ctx.drag_drop_ctx.state = DragAndDropState_Dragging;
			ctx.dock_ctx.payload.docked_target = tab_window;

			push_parent(ctx.general_overlay_widget);

			const auto drag_tab_widget = create_widget("drab_tab_widget");

			if (is_new(*drag_tab_widget)) {
				drag_tab_widget->entity
					.set<Node>({
						.border_radius = 4.f,
						.border_width = 2.f,
					})
					.set<ImmediateId>({ drag_tab_widget->id })
					.set<BackgroundColor>(Color::from_uint(40, 40, 40, 255))
					.set<BorderColor>(Color::from_hex("#d65d0e"))
					.set(GrowDirection::Horizontal)
					.set<SizeStrategy>({
						.x = Fixed{ 100.f },
						.y = Fixed { 28.f },
					})
					.set<Text>(std::string(window.name))
					.set<TextFont>({
						.handle = ctx.world->get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
						.size = 24,
					})
					.set<TextColor>(WHITE);
			}

			auto& transform = drag_tab_widget->entity.ensure<Transform>();

			transform.translation = glm::vec3{ ctx.mouse_input.position, PREVIEW_WIDGET_LAYER };
			tab_name_color = Color::from_hex("#928374");

			pop_parent();
		}
		else {
			tab_name_color = WHITE;
		}

		pop_parent();
	}

	if (is_new(*tabsbar_widget)) {
		tabsbar_widget->entity
			.set<Node>({
				.child_alignment = { 0.f, 1.f },
			})
			.set<ImmediateId>({ tabsbar_widget->id })
			.set<BackgroundColor>(Color::from_hex("#928374"))
			.set(GrowDirection::Horizontal)
			.set<SizeStrategy>({
				.x = Grow{},
				.y = Fixed{ 28.f },
			});
	}

	pop_parent();
}

void Immediate::dock_inner_options(const DockTarget& docking_target) {
	const auto [docking_id, docking_pos, docking_size] = se::visit(docking_target, se::visitors{
		[](Window* window) {
			return std::tuple{ window->id, window->pos, window->size };
		},
		[](DockNode* node) {
			return std::tuple{ node->id, node->position, node->size };
		}
	});

	push_parent(ctx.general_overlay_widget);

	constexpr auto name_format = "dockspace_options_{}";
	const auto background_color = Color::from_uint(69, 133, 136, 180);
	const auto border_radius = 4.f;
	const auto size = std::min(150.f, std::min(docking_size.x, docking_size.y) * 0.65f);

	const auto holder_widget = create_widget(std::format("dock_inner_options_holder_{}", docking_id));

	push_parent(holder_widget);

	const auto options_widget = create_widget(std::format("dock_inner_options_{}", docking_id));

	push_parent(options_widget);

	const auto top_row_widget = create_widget(std::format("top_row_options_{}", docking_id));
	const auto mid_row_widget = create_widget(std::format("mid_row_options_{}", docking_id));
	const auto bot_row_widget = create_widget(std::format("bot_row_options_{}", docking_id));

	push_parent(top_row_widget);

	const auto top_left_widget = create_widget(std::format("top_left_options_{}", docking_id));
	const auto top_mid_widget = create_widget(std::format("top_mid_options_{}", docking_id));
	const auto top_right_widget = create_widget(std::format("top_right_options_{}", docking_id));

	pop_parent();

	push_parent(mid_row_widget);

	const auto mid_left_widget = create_widget(std::format("mid_left_options_{}", docking_id));
	const auto mid_mid_widget = create_widget(std::format("mid_mid_options_{}", docking_id));
	const auto mid_right_widget = create_widget(std::format("mid_right_options_{}", docking_id));

	pop_parent();

	push_parent(bot_row_widget);

	const auto bot_left_widget = create_widget(std::format("bot_left_options_{}", docking_id));
	const auto bot_mid_widget = create_widget(std::format("bot_mid_options_{}", docking_id));
	const auto bot_right_widget = create_widget(std::format("bot_right_options_{}", docking_id));

	pop_parent();

	pop_parent();

	pop_parent();

	pop_parent();

	push_parent(top_mid_widget);

	const auto top_mid_actual_widget = create_widget(std::format("top_mid_options_{}_actual", docking_id));

	pop_parent();

	push_parent(mid_left_widget);

	const auto mid_left_actual_widget = create_widget(std::format("mid_left_options_{}_actual", docking_id));

	pop_parent();

	push_parent(mid_mid_widget);

	const auto mid_mid_actual_widget = create_widget(std::format("mid_mid_options_{}_actual", docking_id));

	pop_parent();

	push_parent(mid_right_widget);

	const auto mid_right_actual_widget = create_widget(std::format("mid_right_options_{}_actual", docking_id));

	pop_parent();

	push_parent(bot_mid_widget);

	const auto bot_mid_actual_widget = create_widget(std::format("bot_mid_options_{}_actual", docking_id));

	pop_parent();

	if (is_new(*holder_widget)) {
		const auto padding = glm::vec4(2.f);

		holder_widget->entity
			.set(GrowDirection::Horizontal)
			.set<Transform>({
				.translation = glm::vec3(docking_pos, DOCK_OPTION_WIDGET_LAYER),
			})
			.set(SizeStrategy{
				.x = Fixed{ docking_size.x },
				.y = Fixed{ docking_size.y },
			})
			.set<Node>({
				.child_alignment = { 0.5f, 0.5f },
				.absolute = true,
			});

		options_widget->entity
			.add<Node>()
			.set(SizeStrategy{
				.x = Fixed{ size },
				.y = Fixed{ size },
			})
			.set(GrowDirection::Vertical);

		top_row_widget->entity
			.add<Node>()
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			});

		mid_row_widget->entity
			.add<Node>()
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			});

		bot_row_widget->entity
			.add<Node>()
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			});

		top_left_widget->entity
			.set(GrowDirection::Horizontal)
			.add<Node>()
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			});

		top_mid_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.padding = padding,
			});

		top_right_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
			});

		mid_left_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.padding = padding,
			});

		mid_mid_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.padding = padding,
			});

		mid_right_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.padding = padding,
			});

		bot_left_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.add<Node>();

		bot_mid_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.padding = padding,
			});

		bot_right_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.add<Node>();

		top_mid_actual_widget->entity
			.set(GrowDirection::Horizontal)
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = 4.f,
			});

		mid_left_actual_widget->entity
			.set(GrowDirection::Horizontal)
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			}) .set<Node>({
				.border_radius = 4.f,
			});

		mid_mid_actual_widget->entity
			.set(GrowDirection::Horizontal)
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = 4.f,
			});
		mid_right_actual_widget->entity
			.set(GrowDirection::Horizontal)
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = 4.f,
			});

		bot_mid_actual_widget->entity
			.set(GrowDirection::Horizontal)
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = 4.f,
			});
	}

	if (is_widget_hovered(*top_mid_widget)) {
		ctx.dock_ctx.payload.split_axis     = SplitAxis_Horizontal;
		ctx.dock_ctx.payload.dock_side      = DockSide_TopLeft;
		ctx.dock_ctx.payload.docking_target = docking_target;
	}
	else if (is_widget_hovered(*bot_mid_widget)) {
		ctx.dock_ctx.payload.split_axis     = SplitAxis_Horizontal;
		ctx.dock_ctx.payload.dock_side      = DockSide_BotRight;
		ctx.dock_ctx.payload.docking_target = docking_target;
	}
	else if (is_widget_hovered(*mid_left_widget)) {
		ctx.dock_ctx.payload.split_axis     = SplitAxis_Vertical;
		ctx.dock_ctx.payload.dock_side      = DockSide_TopLeft;
		ctx.dock_ctx.payload.docking_target = docking_target;
	}
	else if (is_widget_hovered(*mid_right_widget)) {
		ctx.dock_ctx.payload.split_axis     = SplitAxis_Vertical;
		ctx.dock_ctx.payload.dock_side      = DockSide_BotRight;
		ctx.dock_ctx.payload.docking_target = docking_target;
	}
	else if (is_widget_hovered(*mid_mid_widget)) {
		ctx.dock_ctx.payload.split_axis     = SplitAxis_None;
		ctx.dock_ctx.payload.dock_side      = DockSide_Center;
		ctx.dock_ctx.payload.docking_target = docking_target;
	}
}

void Immediate::dock_outer_options(HashId dockspace_id) {
	const auto dockspace_node = get_dock_node(dockspace_id);

	if (!dockspace_node) {
		return;
	}

	push_parent(ctx.general_overlay_widget);

	const auto background_color = Color::from_uint(69, 133, 136, 180);
	const auto border_radius = 4.f;
	const auto size = 150.f;//std::min({ dock_node.size.x, dock_node.size.y, 150.f });

	const auto holder_widget = create_widget(std::format("dock_outer_options_holder_{}", dockspace_id));

	push_parent(holder_widget);

	const auto left_option_widget = create_widget(std::format("left_option_{}", dockspace_id));
	const auto right_option_widget = create_widget(std::format("right_option_{}", dockspace_id));

	if (dockspace_node->is_leaf()) {
		const auto center_option_widget = create_widget(std::format("center_option_{}", dockspace_id));

		if (is_new(*center_option_widget)) {
			center_option_widget->entity
				.set<BackgroundColor>(background_color)
				.set(SizeStrategy{
					.x = Fixed{ 50.f },
					.y = Fixed{ 50.f },
				})
				.set<Node>({
					.self_alignment = { 0.5f, 0.5f },
					.absolute = true,
					.border_radius = border_radius,
				})
				.set(GrowDirection::Horizontal);
		}

		if (is_widget_hovered(*center_option_widget)) {
			ctx.dock_ctx.payload.dock_side      = DockSide_Center;
			ctx.dock_ctx.payload.split_axis     = SplitAxis_None;
			ctx.dock_ctx.payload.docking_target = dockspace_node;
		}
	}

	const auto top_option_widget = create_widget(std::format("top_option_{}", dockspace_id));
	const auto bot_option_widget = create_widget(std::format("bot_option_{}", dockspace_id));

	pop_parent();

	pop_parent();

	if (is_new(*holder_widget)) {
		holder_widget->entity
			.set<Transform>({
				.translation = glm::vec3(dockspace_node->position, DOCK_OPTION_WIDGET_LAYER),
			})
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Fixed{ dockspace_node->size.x },
				.y = Fixed{ dockspace_node->size.y },
			})
			.set<Node>({
				//.child_alignment = { 0.5f, 0.5f },
				.absolute = true,
			});

		left_option_widget->entity
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Fixed{ 50.f },
				.y = Fixed{ 100.f },
			})
			.set<Node>({
				.self_alignment = { 0.f, 0.5f },
				.absolute = true,
				.border_radius = border_radius,
			})
			.set(GrowDirection::Horizontal);

		right_option_widget->entity
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Fixed{ 50.f },
				.y = Fixed{ 100.f },
			})
			.set<Node>({
				.self_alignment = { 1.f, 0.5f },
				.absolute = true,
				.border_radius = border_radius,
			})
			.set(GrowDirection::Horizontal);

		top_option_widget->entity
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Fixed{ 100.f },
				.y = Fixed{ 50.f },
			})
			.set<Node>({
				.self_alignment = { 0.5f, 0.f },
				.absolute = true,
				.border_radius = border_radius,
			})
			.set(GrowDirection::Horizontal);

		bot_option_widget->entity
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Fixed{ 100.f },
				.y = Fixed{ 50.f },
			})
			.set<Node>({
				.self_alignment = { 0.5f, 1.0f },
				.absolute = true,
				.border_radius = border_radius,
			})
			.set(GrowDirection::Horizontal);
	}

	if (is_widget_hovered(*top_option_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis_Horizontal;
		ctx.dock_ctx.payload.dock_side  = DockSide_TopLeft;
		ctx.dock_ctx.payload.docking_target = dockspace_node;
	}
	else if (is_widget_hovered(*bot_option_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis_Horizontal;
		ctx.dock_ctx.payload.dock_side  = DockSide_BotRight;
		ctx.dock_ctx.payload.docking_target = dockspace_node;
	}
	else if (is_widget_hovered(*left_option_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis_Vertical;
		ctx.dock_ctx.payload.dock_side  = DockSide_TopLeft;
		ctx.dock_ctx.payload.docking_target = dockspace_node;
	}
	else if (is_widget_hovered(*right_option_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis_Vertical;
		ctx.dock_ctx.payload.dock_side  = DockSide_BotRight;
		ctx.dock_ctx.payload.docking_target = dockspace_node;
	}
}

void Immediate::dock_preview(const DockTarget& docking_target, const DockTarget& docked_target, SplitAxis split_axis, DockSide dock_side) {
	const auto [docking_id, docking_pos, docking_size] = se::visit(docking_target, se::visitors{
		[](Window* window) {
			return std::tuple{ window->id, window->pos, window->size };
		},
		[](DockNode* node) {
			return std::tuple{ node->id, node->position, node->size };
		}
	});

	const auto docked_aspect = se::visit(docked_target, se::visitors{
		[&](Window* window) {
			auto result = 0.5f;

			if (split_axis == SplitAxis_Horizontal) {
				result = window->size.y / docking_size.y;
			}
			else if (split_axis == SplitAxis_Vertical) {
				result = window->size.x / docking_size.x;
			}

			return std::min(0.5f, std::max(0.15f, result));
		},
		[&](DockNode* node) {
			auto result = 0.5f;

			if (split_axis == SplitAxis_Horizontal) {
				result = node->size.y / docking_size.y;
			}
			else if (split_axis == SplitAxis_Vertical) {
				result = node->size.x / docking_size.x;
			}

			return std::min(0.5f, std::max(0.15f, result));
		}
	});

	push_parent(ctx.general_overlay_widget);

	const auto preview_widget = create_widget(std::format("dock_preview_{}", docking_id));

	const auto preview_color = Color::from_uint(69, 133, 136, 127);

	auto target_pos  = docking_pos;
	auto target_size = docking_size;

	if (split_axis != SplitAxis_None) {
		const auto side_coef = static_cast<float>(dock_side);

		target_size = split_axis == SplitAxis_Horizontal
			? glm::vec2{ docking_size.x, (docking_size.y - ctx.dock_ctx.node_gap) * docked_aspect }
			: glm::vec2{ (docking_size.x - ctx.dock_ctx.node_gap) * docked_aspect, docking_size.y };

		target_pos = split_axis == SplitAxis_Horizontal
			? glm::vec2{ docking_pos.x, docking_pos.y + docking_size.y * side_coef - target_size.y * side_coef }
			: glm::vec2{ docking_pos.x + docking_size.x * side_coef - target_size.x * side_coef, docking_pos.y };
	}

	const auto [edge_pos, edge_size] = [&]() -> std::pair<glm::vec2, glm::vec2> {
		if (dock_side == DockSide_Center) {
			return { docking_pos + docking_size * 0.5f, glm::vec2{ 0.f } };
		}

		const auto side_coef = static_cast<float>(dock_side);

		if (split_axis == SplitAxis_Horizontal) {
			return {
				glm::vec2{ docking_pos.x, docking_pos.y + docking_size.y * side_coef },
				glm::vec2{ docking_size.x, 0.f },
			};
		}

		return {
			glm::vec2{ docking_pos.x + docking_size.x * side_coef, docking_pos.y },
			glm::vec2{ 0.f, docking_size.y },
		};
	}();

	const auto pos_x  = animation_value(std::format("dock_preview_pos_x_{}",  docking_id), edge_pos.x,  target_pos.x,  ctx.anim_pop_rate);
	const auto pos_y  = animation_value(std::format("dock_preview_pos_y_{}",  docking_id), edge_pos.y,  target_pos.y,  ctx.anim_pop_rate);
	const auto size_x = animation_value(std::format("dock_preview_size_x_{}", docking_id), edge_size.x, target_size.x, ctx.anim_pop_rate);
	const auto size_y = animation_value(std::format("dock_preview_size_y_{}", docking_id), edge_size.y, target_size.y, ctx.anim_pop_rate);

	if (is_new(*preview_widget)) {
		preview_widget->entity
			.set<BackgroundColor>(preview_color)
			.set(GrowDirection::Horizontal)
			.set<Node>({ .border_radius = 4.f });
	}

	preview_widget->entity.ensure<Transform>().translation = glm::vec3(pos_x, pos_y, PREVIEW_WIDGET_LAYER);

	auto& size_strategy = preview_widget->entity.ensure<SizeStrategy>();
	size_strategy.x = Fixed{ size_x };
	size_strategy.y = Fixed{ size_y };

	pop_parent();
}

Immediate::DockNode* Immediate::get_dock_node(HashId node_id) {
	if (!ctx.dock_ctx.node_id_to_node.contains(node_id)) {
		// print warning?
		return nullptr;
	}

	return &ctx.dock_ctx.node_id_to_node.at(node_id);
}

Immediate::DockNode* Immediate::get_dock_root_node(HashId node_id) {
	auto node = get_dock_node(node_id);

	while (node->parent_id != 0) {
		node = get_dock_node(node->parent_id);
	}

	return node;
}

Immediate::DockNode* Immediate::create_dock_root(glm::vec2 position, glm::vec2 size) {
	const auto id = ctx.dock_ctx.next_id++;

	ctx.dock_ctx.node_id_to_node[id] = DockNode{
		.id = id,
		.size = size,
		.position = position,
		.root = true,
	};

	ctx.dock_ctx.roots.emplace_back(id);

	return &ctx.dock_ctx.node_id_to_node.at(id);
}

Immediate::DockNode* Immediate::create_dock_root(HashId id, glm::vec2 position, glm::vec2 size) {
	ctx.dock_ctx.node_id_to_node[id] = DockNode{
		.id = id,
		.size = size,
		.position = position,
		.root = true,
	};

	ctx.dock_ctx.roots.emplace_back(id);

	return &ctx.dock_ctx.node_id_to_node.at(id);
}

Immediate::DockNode* Immediate::create_dock_child(DockNode* parent, DockSide side) {
	assert(parent != nullptr);

	auto node = DockNode{
		.parent_id = parent->id,
		.id = ctx.dock_ctx.next_id++,
	};

	parent->children[side == DockSide_TopLeft ? 0 : 1] = node.id;

	ctx.dock_ctx.node_id_to_node[node.id] = node;

	return &ctx.dock_ctx.node_id_to_node[node.id];
}

std::vector<Immediate::HashId> Immediate::get_all_node_windows(DockNode* root) {
	std::vector<HashId> result;

	bfs(root->id, [&](DockNode* node) {
		for (const auto window_id : node->windows) {
			result.push_back(window_id);
		}
	});

	return result;
}

void Immediate::delete_dock_node(HashId dock_node_id) {
	ctx.dock_ctx.node_id_to_node.erase(dock_node_id);
}

void Immediate::dock_node_update_ratio(HashId node_id, float new_ratio) {
	const auto node = get_dock_node(node_id);
	const auto parent_node = get_dock_node(node->parent_id);

	assert(new_ratio <= 1.f && new_ratio >= 0.f);

	node->aspect_ratio = new_ratio;

	if (parent_node) {
		const auto other_node = get_dock_node(parent_node->children[0] == node_id ? parent_node->children[1] : parent_node->children[0]);

		other_node->aspect_ratio = 1.f - new_ratio;
	}
}

void Immediate::dock_node_skip_anims(HashId node_id) {
	bfs(node_id, [](DockNode* node) {
		node->skip_next_anims = true;
	});
}

std::pair<Immediate::DockNode*, Immediate::DockNode*> Immediate::split_node(HashId node_id, SplitAxis split_axis) {
	auto left = DockNode{
		.id = ctx.dock_ctx.next_id++,
		.dock_side = DockSide_TopLeft,
	};
	auto right = DockNode{
		.id = ctx.dock_ctx.next_id++,
		.dock_side = DockSide_BotRight,
	};

	auto parent = get_dock_node(node_id);

	left.parent_id = parent->id;
	left.aspect_ratio = 0.5f;

	right.parent_id = parent->id;
	right.aspect_ratio = 0.5f;

	parent->children[0] = left.id;
	parent->children[1] = right.id;
	parent->split_axis = split_axis;

	ctx.dock_ctx.node_id_to_node[left.id] = left;
	ctx.dock_ctx.node_id_to_node[right.id] = right;

	return { &ctx.dock_ctx.node_id_to_node[left.id], &ctx.dock_ctx.node_id_to_node[right.id] };
}

void Immediate::apply_dock(const DockTarget& docking_target, const DockTarget& docked_target, SplitAxis split_axis, DockSide dock_side) {
	//assert(std::get<0>(docking_target) != nullptr || std::get<1>(docking_target) != nullptr);
	//assert(std::get<0>(docked_target) != nullptr || std::get<1>(docked_target) != nullptr);

	se::visit(docking_target, se::visitors{
		[&](Window* window) {
			auto node = create_dock_root(window->pos, window->size);

			node->host_window = window->id;

			se::visit(docked_target, se::visitors{
				[&](Window* docked_window) {
					if (dock_side == DockSide_Center) {
						node->windows = { window->id, docked_window-> id};
						node->active_window = docked_window->id;

						window->dock_node_id = node->id;
						docked_window->dock_node_id = node->id;
					}
					else {
						const auto [left_node, right_node] = split_node(node->id, split_axis);
						const auto aspect_ratio = split_axis == SplitAxis_Horizontal
							? docked_window->size.y / window->size.y
							: docked_window->size.x / window->size.x;

						if (dock_side == DockSide_TopLeft) {
							left_node->windows.push_back(docked_window->id);
							left_node->active_window = docked_window->id;
							left_node->aspect_ratio = std::min(0.5f, aspect_ratio);

							right_node->windows.push_back(window->id);
							right_node->active_window = window->id;
							right_node->aspect_ratio = 1.f - left_node->aspect_ratio;

							docked_window->dock_node_id = left_node->id;
							window->dock_node_id = right_node->id;
						}
						else {
							right_node->windows.push_back(docked_window->id);
							right_node->active_window = docked_window->id;
							right_node->aspect_ratio = std::min(0.5f, aspect_ratio);

							left_node->windows.push_back(window->id);
							left_node->active_window = window->id;
							left_node->aspect_ratio = 1.f - right_node->aspect_ratio;

							docked_window->dock_node_id = right_node->id;
							window->dock_node_id = left_node->id;
						}
					}
				},
				[&](DockNode* docked_node) {
					const auto docking_node = create_dock_child(node, dock_side == DockSide_TopLeft ? DockSide_BotRight : DockSide_TopLeft);

					docked_node->aspect_ratio = split_axis == SplitAxis_Horizontal
						? docked_node->size.y / window->size.y
						: docked_node->size.x / window->size.x;
					docked_node->aspect_ratio = std::min(0.5f, docked_node->aspect_ratio);

					docking_node->windows.push_back(window->id);
					docking_node->active_window = window->id;
					docking_node->aspect_ratio = 1.f - docked_node->aspect_ratio;

					window->dock_node_id = docking_node->id;

					docked_node->parent_id = node->id;

					node->children[static_cast<std::uint8_t>(dock_side)] = docked_node->id;
				}
			});
		},
		[&](DockNode* node) {
			se::visit(docked_target, se::visitors{
				[&](Window* window) {
					if (dock_side == DockSide_Center) {
						node->windows.push_back(window->id);
						node->active_window = window->id;

						window->dock_node_id = node->id;
					}
					else {
						const auto node_windows       = node->windows;
						const auto node_split         = node->split_axis;
						const auto node_children      = node->children;
						const auto node_active_window = node->active_window;

						node->windows.clear();
						node->active_window = 0;

						const auto [left_node, right_node] = split_node(node->id, split_axis);

						const auto aspect_ratio = split_axis == SplitAxis_Horizontal
							? window->size.y / node->size.y
							: window->size.x / node->size.x;

						if (dock_side == DockSide_TopLeft) {
							left_node->windows.push_back(window->id);
							left_node->active_window = window->id;
							left_node->aspect_ratio = std::min(0.5f, aspect_ratio);

							right_node->aspect_ratio = 1.f - left_node->aspect_ratio;
							right_node->children = node_children;
							right_node->windows = node_windows;
							right_node->active_window = node_active_window;
							right_node->split_axis = node_split;

							if (node->central_node) {
								right_node->central_node = true;
								node->central_node = false;

								get_dock_root_node(node->id)->central_node_id = right_node->id;
							}

							window->dock_node_id = left_node->id;

							for (auto child_id : node_children) {
								if (child_id == 0) {
									continue;
								}

								get_dock_node(child_id)->parent_id = right_node->id;
							}

							for (const auto window_id : node_windows) {
								ctx.id_to_window.at(window_id).dock_node_id = right_node->id;
							}
						}
						else {
							right_node->windows.push_back(window->id);
							right_node->active_window = window->id;
							right_node->aspect_ratio = std::min(0.5f, aspect_ratio);

							left_node->aspect_ratio = 1.f - right_node->aspect_ratio;
							left_node->children = node_children;
							left_node->windows = node_windows;
							left_node->active_window = node_active_window;
							left_node->split_axis = node_split;

							if (node->central_node) {
								left_node->central_node = true;
								node->central_node = false;

								get_dock_root_node(node->id)->central_node_id = left_node->id;
							}

							window->dock_node_id = right_node->id;

							for (auto child_id : node_children) {
								if (child_id == 0) {
									continue;
								}

								get_dock_node(child_id)->parent_id = left_node->id;
							}

							for (const auto window_id : node_windows) {
								ctx.id_to_window.at(window_id).dock_node_id = left_node->id;
							}
						}
					}
				},
				[&](DockNode* docked_node) {
					if (dock_side == DockSide_Center) {
						node->windows.append_range(docked_node->windows);
						node->active_window = docked_node->active_window;

						for (const auto window_id : docked_node->windows) {
							auto& window = ctx.id_to_window.at(window_id);

							window.dock_node_id = node->id;
						}

						docked_node->windows.clear();
						docked_node->active_window = 0;
					}
					else {
						const auto other_child = create_dock_child(node, dock_side == DockSide_TopLeft ? DockSide_BotRight : DockSide_TopLeft);

						const auto docked_index = dock_side == DockSide_TopLeft ? 0 : 1;
						const auto aspect_ratio = split_axis == SplitAxis_Horizontal
							? docked_node->size.y / node->size.y
							: docked_node->size.x / node->size.x;

						node->split_axis                 = split_axis;
						node->children[    docked_index] = docked_node->id;
						node->children[1 - docked_index] = other_child->id;

						docked_node->parent_id    = node->id;
						docked_node->aspect_ratio = std::min(0.5f, aspect_ratio);

						other_child->aspect_ratio = 1.f - docked_node->aspect_ratio;

						if (node->central_node) {
							other_child->central_node = true;
							node->central_node = false;

							get_dock_root_node(node->id)->central_node_id = other_child->id;
						}
					}
				}
			});
		}
	});
}

void Immediate::undock_window(Window& window) {
	if (!window.dock_node_id) {
		return;
	}

	auto dock_node = get_dock_node(window.dock_node_id.value());

	window.dock_node_id.reset();

	if (!dock_node) {
		return;
	}

	std::erase(dock_node->windows, window.id);

	if (dock_node->active_window == window.id) {
		dock_node->active_window = 0;
	}

	if (!dock_node->windows.empty()) {
		dock_node->active_window = dock_node->windows.back();
	}
}

void Immediate::update_dock_nodes() {
	std::vector<HashId> nodes_to_delete;
	std::vector<HashId> roots_to_delete;

	for (auto node_id : ctx.dock_ctx.roots) {
		const auto root = get_dock_node(node_id);

		bfs(node_id, [&](DockNode* node) {
			if (node->parent_id != 0 && node->root) {
				roots_to_delete.push_back(node->id);

				node->root = false;
			}

			const auto leaf_node = node->children[0] == 0;

			if (leaf_node && node->windows.empty() && node->central_node_id == 0 && !node->central_node) {
				nodes_to_delete.push_back(node->id);

				return;
			}

			if (leaf_node) {
				if (node->parent_id == 0 && node->windows.size() == 1 && !node->dockspace) {
					nodes_to_delete.push_back(node->id);
				}
				else if (!node->windows.empty()) {
					for (auto window : node->windows) {
						ctx.id_to_window.at(window).disabled = true;
					}

					if (node->next_active_window != 0) {
						node->active_window = node->next_active_window;
						node->next_active_window = 0;
					}

					ctx.id_to_window.at(node->active_window).disabled = false;
				}
			}
			else {
				const auto size = node->size - ctx.dock_ctx.node_gap;

				for (size_t i = 0; i < node->children.size(); ++i) {
					const auto child_id = node->children[i];

					const auto multiplier = static_cast<float>(i);
					const auto child = get_dock_node(child_id);

					child->position = node->position;

					if (node->split_axis == SplitAxis_Horizontal) {
						child->size.x = node->size.x;
						child->size.y = size.y * child->aspect_ratio;
						child->position.y += ctx.dock_ctx.node_gap * multiplier + size.y * multiplier - child->size.y * multiplier;
					}
					else {
						child->size.x = size.x * child->aspect_ratio;
						child->size.y = node->size.y;
						child->position.x += ctx.dock_ctx.node_gap * multiplier + size.x * multiplier - child->size.x * multiplier;
					}
				}
			}
		});
	}

	for (const auto id : nodes_to_delete) {
		auto node = get_dock_node(id);
		auto root_it = std::ranges::find(ctx.dock_ctx.roots, id);

		if (root_it != ctx.dock_ctx.roots.end()) {
			ctx.dock_ctx.roots.erase(root_it);
		}

		if (node->parent_id != 0) {
			auto parent = get_dock_node(node->parent_id);

			if (parent->children[0] == node->id) {
				const auto other_child = get_dock_node(parent->children[1]);

				parent->split_axis = other_child->split_axis;
				parent->windows = other_child->windows;
				parent->active_window = other_child->active_window;
				parent->children = other_child->children;

				for (const auto child_id : other_child->children) {
					if (child_id == 0) {
						continue;
					}

					get_dock_node(child_id)->parent_id = parent->id;
				}

				if (other_child->central_node) {
					parent->central_node = other_child->central_node;
					get_dock_root_node(parent->id)->central_node_id = parent->id;
				}

				ctx.dock_ctx.node_id_to_node.erase(other_child->id);
			}
			else {
				const auto other_child = get_dock_node(parent->children[0]);

				parent->split_axis = other_child->split_axis;
				parent->windows = other_child->windows;
				parent->active_window = other_child->active_window;
				parent->children = other_child->children;

				for (const auto child_id : other_child->children) {
					if (child_id == 0) {
						continue;
					}

					get_dock_node(child_id)->parent_id = parent->id;
				}

				if (other_child->central_node) {
					parent->central_node = other_child->central_node;
					get_dock_root_node(parent->id)->central_node_id = parent->id;
				}

				ctx.dock_ctx.node_id_to_node.erase(other_child->id);
			}

			if (parent->parent_id == 0 && !parent->dockspace && parent->windows.size() < 2) {
				for (const auto window_id : parent->windows) {
					if (ctx.id_to_window.contains(window_id)) {
						auto& window = ctx.id_to_window.at(window_id);

						window.dock_node_id.reset();
						window.disabled = false;
					}
				}

				if (parent->root) {
					roots_to_delete.push_back(parent->id);
				}

				ctx.dock_ctx.node_id_to_node.erase(parent->id);
			}
			else {
				for (const auto window_id : parent->windows) {
					if (ctx.id_to_window.contains(window_id)) {
						ctx.id_to_window.at(window_id).dock_node_id = parent->id;
					}
				}
			}
		}
		else {
			for (const auto window_id : node->windows) {
				if (ctx.id_to_window.contains(window_id)) {
					auto& window = ctx.id_to_window.at(window_id);

					window.dock_node_id.reset();
					window.disabled = false;
				}
			}

			if (node->root) {
				roots_to_delete.push_back(node->id);
			}
		}

		ctx.dock_ctx.node_id_to_node.erase(id);
	}

	for (auto root_id : roots_to_delete) {
		std::erase(ctx.dock_ctx.roots, root_id);
	}
}

void Immediate::bfs(HashId root, const std::function<void(DockNode*)>& callback) {
	std::queue<HashId> queue;

	queue.push(root);

	while (!queue.empty()) {
		const auto node_id = queue.front();
		const auto node = get_dock_node(node_id);
		queue.pop();

		callback(get_dock_node(node_id));

		for (auto child_id : node->children) {
			if (child_id == 0) {
				continue;
			}

			queue.push(child_id);
		}
	}
}

void Immediate::focus_window(Window& window) {
	for (size_t i = window.focus_order + 1; i < ctx.windows_focus_order.size(); ++i) {
		--ctx.windows_focus_order[i]->focus_order;
	}

	for (auto& [_, window] : ctx.id_to_window) {
		if (window.disabled) {
			continue;
		}

		window.has_focus = false;
	}

	window.focus_order = ctx.id_to_window.size() - 1;
	window.has_focus = true;
}

void Immediate::grip_window_borders(Window& window) {
	push_parent(window.overlay_id);

	for (int dir = 0; dir < 4; ++dir) {
		const auto grip_widget = create_widget(std::format("{}_{}_grip", window.name, dir), WidgetFlags_Clickable);

		if (is_new(*grip_widget)) {
			grip_widget->entity
				.set(GrowDirection::Horizontal)
				.set<Node>({
					.absolute = true,
				});
		}

		auto& color      = grip_widget->entity.ensure<BackgroundColor>();
		auto& transform  = grip_widget->entity.ensure<Transform>();
		auto& size       = grip_widget->entity.ensure<SizeStrategy>();

		if (dir % 2 == 0) {
			transform.translation.x = dir == 0 ? -1.f : window.size.x - 2.f;

			size.x = Fixed{ 3.f };
			size.y = Fixed{ window.size.y };
		}
		else {
			transform.translation.y = dir == 1 ? -1.f : window.size.y - 2.f;

			size.x = Fixed{ window.size.x };
			size.y = Fixed{ 3.f };
		}

		if (drag_interaction(grip_widget)) {
			const auto delta = ctx.mouse_input.position - last_ctx.mouse_input.position;

			color = Color::from_hex("#fbf1c7");

			ctx.cursor_type = dir % 2 == 0 ? SDL_SYSTEM_CURSOR_EW_RESIZE : SDL_SYSTEM_CURSOR_NS_RESIZE;

			if (dir % 2 == 0) {
				window.pos.x += delta.x * (dir == 0 ? 1.f : 0.f);
				window.size.x += delta.x * (dir == 0 ? -1.f : 1.f);
			}
			else {
				window.pos.y += delta.y * (dir == 1 ? 1.f : 0.f);
				window.size.y += delta.y * (dir == 1 ? -1.f : 1.f);
			}
		}
		else if (hover_interaction(grip_widget)) {
			color = Color::from_hex("#d5c4a1");

			ctx.cursor_type = dir % 2 == 0 ? SDL_SYSTEM_CURSOR_EW_RESIZE : SDL_SYSTEM_CURSOR_NS_RESIZE;
		}
		else {
			color = TRANSPARENT;
		}
	}

	pop_parent();
}
void Immediate::grip_dock_borders(DockNode& dock_node) {
	if (dock_node.split_axis == SplitAxis_None && dock_node.windows.empty()) {
		return;
	}

	push_parent(dock_node.overlay_widget_id);

	const auto& host_window = dock_node.windows.size() > 1
		? ctx.id_to_window[dock_node.active_window]
		: ctx.id_to_window[dock_node.host_window];

	if (!static_cast<bool>(host_window.flags & WindowFlags_NoResize)) {
		for (int dir = 0; dir < 4; ++dir) {
			const auto grip_widget = create_widget(std::format("{}_{}_grip", host_window.name, dir), WidgetFlags_Clickable);

			if (is_new(*grip_widget)) {
				grip_widget->entity
					.set(GrowDirection::Horizontal)
					.set<Node>({
						.absolute = true,
					});
			}

			auto& color      = grip_widget->entity.ensure<BackgroundColor>();
			auto& transform  = grip_widget->entity.ensure<Transform>();
			auto& size       = grip_widget->entity.ensure<SizeStrategy>();

			if (dir % 2 == 0) {
				transform.translation.x = dir == 0 ? -1.f : dock_node.size.x - 2.f;

				size.x = Fixed{ 3.f };
				size.y = Fixed{ dock_node.size.y };
			}
			else {
				transform.translation.y = dir == 1 ? -1.f : dock_node.size.y - 2.f;

				size.x = Fixed{ dock_node.size.x };
				size.y = Fixed{ 3.f };
			}

			if (drag_interaction(grip_widget)) {
				const auto delta = ctx.mouse_input.position - last_ctx.mouse_input.position;

				color = Color::from_hex("#fbf1c7");

				ctx.cursor_type = dir % 2 == 0 ? SDL_SYSTEM_CURSOR_EW_RESIZE : SDL_SYSTEM_CURSOR_NS_RESIZE;

				if (dir % 2 == 0) {
					dock_node.position.x += delta.x * (dir == 0 ? 1.f : 0.f);
					dock_node.size.x += delta.x * (dir == 0 ? -1.f : 1.f);
				}
				else {
					dock_node.position.y += delta.y * (dir == 1 ? 1.f : 0.f);
					dock_node.size.y += delta.y * (dir == 1 ? -1.f : 1.f);
				}

				dock_node_skip_anims(dock_node.id);
			}
			else if (hover_interaction(grip_widget)) {
				color = Color::from_hex("#d5c4a1");

				ctx.cursor_type = dir % 2 == 0 ? SDL_SYSTEM_CURSOR_EW_RESIZE : SDL_SYSTEM_CURSOR_NS_RESIZE;
			}
			else {
				color = TRANSPARENT;
			}
		}
	}

	bfs(dock_node.id, [&](DockNode* node) {
		if (node->split_axis == SplitAxis_None) {
			return;
		}

		const auto child = get_dock_node(node->children[0]);
		const auto grip_widget = create_widget(std::format("{}_grip", node->id), WidgetFlags_Clickable);

		if (is_new(*grip_widget)) {
			grip_widget->entity
				.set(GrowDirection::Horizontal)
				.set<Node>({
					.absolute = true,
				});
		}

		auto& color      = grip_widget->entity.ensure<BackgroundColor>();
		auto& transform  = grip_widget->entity.ensure<Transform>();
		auto& size       = grip_widget->entity.ensure<SizeStrategy>();

		if (node->split_axis == SplitAxis_Horizontal) {
			transform.translation.y = node->position.y - dock_node.position.y + (node->size.y - ctx.dock_ctx.node_gap) * child->aspect_ratio;
			transform.translation.x = node->position.x - dock_node.position.x;
			size.x = Fixed{ node->size.x };
			size.y = Fixed{ ctx.dock_ctx.node_gap * 2.f };
		}
		else {
			transform.translation.x = node->position.x - dock_node.position.x + (node->size.x - ctx.dock_ctx.node_gap) * child->aspect_ratio;
			transform.translation.y = node->position.y - dock_node.position.y;
			size.x = Fixed{ ctx.dock_ctx.node_gap * 2.f };
			size.y = Fixed{ node->size.y };
		}

		if (drag_interaction(grip_widget)) {
			const auto ratio = node->split_axis == SplitAxis_Horizontal
				? std::min(std::max(ctx.mouse_input.position.y - node->position.y, 0.f), node->size.y) / node->size.y
				: std::min(std::max(ctx.mouse_input.position.x - node->position.x, 0.f), node->size.x) / node->size.x;

			color = Color::from_hex("#fbf1c7");

			ctx.cursor_type = node->split_axis == SplitAxis_Horizontal ? SDL_SYSTEM_CURSOR_NS_RESIZE : SDL_SYSTEM_CURSOR_EW_RESIZE;

			dock_node_update_ratio(child->id, ratio);
			dock_node_skip_anims(node->id);
		}
		else if (hover_interaction(grip_widget)) {
			color = Color::from_hex("#d5c4a1");

			ctx.cursor_type = node->split_axis == SplitAxis_Horizontal ? SDL_SYSTEM_CURSOR_NS_RESIZE : SDL_SYSTEM_CURSOR_EW_RESIZE;
		}
		else {
			color = TRANSPARENT;
		}
	});

	pop_parent();
}
