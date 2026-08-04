#include "immediate.h"

#include "ecsModule/textModule/module.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "gtx/easing.hpp"
#include "utils/visit.h"

#include <format>
#include <queue>
#include <ranges>

using namespace ps::editor;

constexpr auto initial_window_size = glm::vec2{ 100.f, 100.f };

constexpr float DEFAULT_WIDGET_LAYER     = 0.0f;
constexpr float FOCUSED_WIDGET_LAYER     = 0.5f;
constexpr float PREVIEW_WIDGET_LAYER     = 1.0f;
constexpr float DOCK_OPTION_WIDGET_LAYER = 2.0f;

// TODO:
// [x] PushID()/PopID()
// [x] PushStyle()/PopStyle()
// [x] Docking
// [x] remove drag/drop_id

void Immediate::init(flecs::world& world) {
	ctx.world = &world;
	ctx.root = world.entity("immediate_root").add(flecs::OrderedChildren);
	ctx.update_query = world.query_builder<Immediate, ImmediateId, Node, SizeStrategy, Transform, GlobalTransform>().term_at(0).inout().build();
}

void Immediate::begin(std::string name, int flags, std::optional<glm::vec2> pos, std::optional<glm::vec2> size) {
	const auto id = hash(name);

	if (!ctx.id_to_window.contains(id)) {
		ctx.id_to_window[id] = {
			.id = id,
			//.entity = window_widget->entity,
			.name = name,
			.size = initial_window_size,
			.flags = (Window::Flags)flags,
		};
	}

	auto& window = ctx.id_to_window[id];

	ctx.active_window = &window;
	ctx.windows.push_back(&window);

	if (window.dock_node_id) {
		const auto dock_node = get_dock_node(window.dock_node_id.value());

		window.pos = dock_node->position;
		window.size = dock_node->size;
	}

	if (window.disabled) {
		return;
	}

	auto window_widget = create_widget(name, WidgetFlags::Clickable);

	push_parent(window_widget);

	const auto no_titlebar = flags & Window::Flags::NoTitlebar;
	const auto no_move = flags & Immediate::Window::Flags::NoMove;
	const auto no_resize = flags & Immediate::Window::Flags::NoResize;

	if (is_new(*window_widget)) {
		window_widget->entity
			.set<BackgroundColor>(Color::from_uint(40, 40, 40, 255))
			.set<BorderColor>(Color::from_hex("#504945"))
			.set(GrowDirection::Vertical)
			.set<Node>({
				.padding = { 4.f, 4.f, 4.f, 4.f },
				.absolute = true,
				.border_radius = 4.f,
				.border_width = 1.f,
			});

		if (size) {
			window_widget->entity
				.set<SizeStrategy>({
					.x = Fixed{ size.value().x },
					.y = Fixed{ size.value().y },
				});

			window.size = size.value();
		}
		else {
			window_widget->entity
				.set<SizeStrategy>({
					.x = Fixed{ initial_window_size.x },
					.y = Fixed{ initial_window_size.y },
				});
		}

		if (pos) {
			window_widget->entity.set<Transform>({
				.translation  = glm::vec3(pos.value(), 0.f),
			});

			window.pos = pos.value();
		}
	}

	if (!no_move && drag_interaction(window_widget)) {
		const auto delta = ctx.mouse_input.position - last_ctx.mouse_input.position;

		if (window.dock_node_id) {
			get_dock_root_node(window.dock_node_id.value())->position += delta;
		}
		else {
			window.pos += delta;
		}
	}

	if (!no_titlebar) {
		if (window.dock_node_id) {
			tabsbar(window);
		}
		else {
			titlebar(window);
		}
	}

	if (!window.collapsed) {
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

		if (!no_resize && !window.dock_node_id) {
			auto resize_button_widget = create_widget(std::format("{}_resize_button", name), WidgetFlags::Clickable);

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
	}
}

void Immediate::end() {
	if (!ctx.active_window->disabled) {
		if (!ctx.active_window->collapsed) {
			pop_parent(); // content entity
		}
		pop_parent(); // window entity
	}

	ctx.active_window = nullptr;
}

void Immediate::dockspace() {
	auto& active_window = *ctx.active_window;

	if (active_window.disabled || active_window.collapsed) {
		return;
	}

	const auto dockspace_widget = create_widget("dokspace", WidgetFlags::Clickable);
	const auto dock_node = !active_window.dock_node_id
		? create_dock_root(active_window.pos, active_window.size)
		: get_dock_node(active_window.dock_node_id.value());

	if (is_new(*dockspace_widget)) {
		dockspace_widget->entity
			.set<ImmediateId>({ dockspace_widget->id })
			.set<BackgroundColor>(Color::from_uint(40, 40, 40, 255))
			.set<BorderColor>(Color::from_hex("#504945"))
			.set(GrowDirection::Horizontal)
			.set<SizeStrategy>({
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.padding = { 4.f, 4.f, 4.f, 4.f },
				.border_radius = 4.f,
				.border_width = 1.f,
			});

		dock_node->dockspace = true;
		dock_node->central_node = true;
		dock_node->central_node_id = dock_node->id;
		dock_node->widget_id = dockspace_widget->id;

		active_window.dock_node_id = dock_node->id;
	}
}

bool Immediate::button(std::string name) {
	const auto& active_window = *ctx.active_window;

	if (active_window.disabled || active_window.collapsed) {
		return false;
	}

	auto button_widget = create_widget(name, WidgetFlags::Clickable);

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
	const auto& active_window = *ctx.active_window;

	if (active_window.disabled || active_window.collapsed) {
		return;
	}

	auto text_widget = create_widget(text);

	if (is_new(*text_widget)) {
		text_widget->entity
			.set<SizeStrategy>({
				.x = Fixed{ 10.f },
				.y = Fixed{ 10.f },
			})
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

	ctx.mouse_input = input.mouse;
}

void Immediate::end_frame() {
	assert(ctx.parent_stack.empty());

	process_drag_drop();

	if (clear_on_frame_end) {
		clear_inactive_widgets();
		clear_inactive_animations();
	}

	update_dock_nodes();

	calculate_dfs_indices();
	calculate_bfs_indices();

	ctx.update_query.run([](flecs::iter& it) {
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

					if (window.collapsed) {
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

	last_ctx = ctx;
	ctx.windows.clear();
	ctx.widgets.clear();
	ctx.roots.clear();

	ctx.hot_widget = nullptr;

	ctx.widget_num = 0;

	ctx.active_window = nullptr;
	//ctx.dock_ctx.payload.docked_target.reset();

	++ctx.frame_count;
}

float Immediate::animation_value(std::string name, float initial, float target, float rate) {
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

	if (flags & WidgetFlags::Clickable) {
		const auto& mouse_input = ctx.mouse_input;

		if (mouse_input.position.x > widget->last_rect.x && mouse_input.position.x < widget->last_rect.z &&
			mouse_input.position.y > widget->last_rect.y && mouse_input.position.y < widget->last_rect.w
		) {
			ctx.hot_widget = widget;
		}
	}

	return widget;
}

void Immediate::insert_widget_in_tree(Widget* widget, Widget* parent) {
	ctx.widgets.push_back(widget);

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
		ctx.roots.push_back(widget);
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

void Immediate::calculate_dfs_indices() {
	if (ctx.widgets.empty()) {
		return;
	}

	size_t dfs_index = 0;

	for (const auto& root : ctx.roots) {
		dfs(root, [&](Widget* widget) {
			if (!widget) {
				return;
			}

			auto& index = widget->entity.ensure<NodeIndex>();

			index.dfs = dfs_index++;
			index.external_dfs_source = true;
		});
	}
}

void Immediate::calculate_bfs_indices() {
	if (ctx.widgets.empty()) {
		return;
	}

	size_t bfs_index = 0;

	bfs(ctx.roots, [&](Widget* widget) {
			if (!widget) {
				return;
			}

			auto& index = widget->entity.ensure<NodeIndex>();

			index.bfs = bfs_index++;
			index.external_bfs_source = true;
		}
	);
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
	if (ctx.drag_drop_ctx.state == DragAndDropContext::State::None) {
		return;
	}

	const auto& mouse_input = ctx.mouse_input;
	auto& payload = ctx.dock_ctx.payload;

	for (const auto& window : ctx.windows | std::ranges::views::reverse) {
		if (window->disabled || window->collapsed) {
			continue;
		}

		const auto skip = ps::visit(payload.docked_target.value(), ps::visitors{
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
			payload.docking_target = get_dock_node(window->dock_node_id.value());
		}
		else {
			payload.docking_target = window;
		}

		break;
	}


	if (ctx.drag_drop_ctx.state == DragAndDropContext::State::Dragging) {
		payload.dock_side = DockSide::None;
		payload.split_axis = SplitAxis::None;

		ps::visit(payload.docking_target.value(), ps::visitors{
			[&](Window* window) {
				dock_inner_options(window);
			},
			[&](DockNode* node) {
				auto root_node = get_dock_root_node(node->id);

				if (node != root_node) {
					dock_inner_options(node);
				}
				dock_outer_options(node->id);
			}
		});

		if (payload.split_axis != SplitAxis::None || payload.dock_side != DockSide::None) {
			dock_preview(payload.docking_target.value(), payload.docked_target.value(), payload.split_axis, payload.dock_side);
		}
	}
	else if (ctx.drag_drop_ctx.state == DragAndDropContext::State::Dropping) {
		if (ctx.dock_ctx.payload.split_axis != SplitAxis::None || ctx.dock_ctx.payload.dock_side != DockSide::None) {
			apply_dock(payload.docking_target.value(), payload.docked_target.value(), ctx.dock_ctx.payload.split_axis, ctx.dock_ctx.payload.dock_side);
		}
		//else if (ctx.id_to_window.at(ctx.drag_drop_ctx.drag_id).dock_node_id) {
		//	auto& window = ctx.id_to_window.at(ctx.drag_drop_ctx.drag_id);

		//	undock_window(window);

		//	window.pos = ctx.mouse_input.position;
		//}

		payload.dock_side = DockSide::None;
		payload.split_axis = SplitAxis::None;
		payload.docked_target.reset();
		payload.docking_target.reset();

		ctx.drag_drop_ctx.state = DragAndDropContext::State::None;
	}

	if (mouse_input.left.released) {
		ctx.drag_drop_ctx.state = DragAndDropContext::State::Dropping;
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

bool Immediate::is_widget_pressed(const Widget& widget) {
	if (widget.id == 0) {
		return false;
	}

	const auto& mouse_input = ctx.mouse_input;

	if (!mouse_input.left.pressed) {
		return false;
	}

	if (mouse_input.position.x <= widget.last_rect.x || mouse_input.position.x >= widget.last_rect.z ||
		mouse_input.position.y <= widget.last_rect.y || mouse_input.position.y >= widget.last_rect.w
	) {
		return false;
	}

	return ctx.active_widget->id == widget.id;
}

bool Immediate::is_widget_released(const Widget& widget) {
	return false;
	//if (widget.id == 0) {
	//	return false;
	//}

	//const auto& mouse_input = ctx.mouse_input;

	//if (!ctx.last_active_widget) {
	//	return false;
	//}

	//if (!mouse_input.left.pressed) {
	//	return false;
	//}

	//if (mouse_input.position.x <= widget.last_rect.x || mouse_input.position.x >= widget.last_rect.z ||
	//	mouse_input.position.y <= widget.last_rect.y || mouse_input.position.y >= widget.last_rect.w
	//) {
	//	return false;
	//}

	//return ctx.last_active_widget.value()->id == widget.id;
}

bool Immediate::is_widget_down(const Widget& widget) {
	return false;
	//if (widget.id == 0) {
	//	return false;
	//}

	//const auto& mouse_input = ctx.mouse_input;

	//if (!ctx.last_active_widget) {
	//	return false;
	//}

	//if (!mouse_input.left.remain) {
	//	return false;
	//}

	//if (ctx.last_active_widget.value()->id == widget.id) {
	//	return true;
	//}

	//if (mouse_input.position.x <= widget.last_rect.x || mouse_input.position.x >= widget.last_rect.z ||
	//	mouse_input.position.y <= widget.last_rect.y || mouse_input.position.y >= widget.last_rect.w
	//) {
	//	return false;
	//}

	//return ctx.last_active_widget.value()->id == widget.id;
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
	}
	else if (ctx.hot_widget == widget && ctx.mouse_input.left.pressed) {
		ctx.active_widget = widget;
	}

	return result;
}

bool Immediate::drag_offset_interaction(Widget* widget, glm::vec2 offset) {
	bool result = false;

	if (ctx.active_widget == widget) {
		if (ctx.mouse_input.left.remain) {
			const auto delta = glm::abs(ctx.drag_drop_ctx.start_pos - ctx.mouse_input.position);

			result = delta.x > offset.x && delta.y > offset.y;
		}
		else if (ctx.mouse_input.left.released) {
			result = false;

			ctx.active_widget = nullptr;
		}
	}
	else if (ctx.hot_widget == widget && ctx.mouse_input.left.pressed) {
		ctx.active_widget = widget;
		ctx.drag_drop_ctx.start_pos = ctx.mouse_input.position;
	}

	return result;
}

bool Immediate::button_interaction(Widget* widget) {
	bool result = false;

	if (ctx.active_widget == widget && ctx.mouse_input.left.released && ctx.hot_widget == widget) {
		result = true;

		ctx.active_widget = nullptr;
	}
	else if (ctx.hot_widget == widget && ctx.mouse_input.left.pressed) {
		ctx.active_widget = widget;
	}

	return result;
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
	const auto window_entity = ctx.world->entity(window.entity);
	const auto name = window.name;

	const auto no_move = window.flags & Window::Flags::NoMove;
	const auto no_collapse = window.flags & Window::Flags::NoCollapse;
	const auto no_close = window.flags & Window::Flags::NoClose;

	auto titlebar_widget = create_widget(std::format("{}_titlebar", name), WidgetFlags::Clickable);

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

		ctx.drag_drop_ctx.state = DragAndDropContext::State::Dragging;

		ctx.dock_ctx.payload.docked_target = &window;
	}

	if (!no_collapse) {
		auto collapse_button_widget = create_widget(std::format("{}_collapse_button", name), WidgetFlags::Clickable);

		if (is_new(*collapse_button_widget)) {
			collapse_button_widget->entity
				.add<Node>()
				.set(GrowDirection::Horizontal)
				.set<SizeStrategy>({
					.x = Fixed{ 28.f },
					.y = Fixed{ 28.f },
				})
				.set<BackgroundColor>(Color::from_hex("#282828"));
		}

		if (button_interaction(collapse_button_widget)) {
			window.collapsed = !window.collapsed;
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
		auto close_button_widget = create_widget(std::format("{}_close_button", name), WidgetFlags::Clickable);

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

	const auto tabsbar_widget = create_widget(std::format("{}_tabsbar", window.name), WidgetFlags::Clickable);
	const auto& window_ids = dock_node->windows;
	const auto active_window = dock_node->active_window;

	if (drag_interaction(tabsbar_widget)) {
		const auto root_node = get_dock_root_node(dock_node->id);

		root_node->position += ctx.mouse_input.position - last_ctx.mouse_input.position;

		ctx.drag_drop_ctx.state = DragAndDropContext::State::Dragging;
		ctx.dock_ctx.payload.docked_target = root_node;
	}

	push_parent(tabsbar_widget);

	for (const auto window_id : window_ids) {
		const auto window = &ctx.id_to_window.at(window_id);
		const auto tab_widget = create_widget(std::format("{}_tab", window->name), WidgetFlags::Clickable);
		const auto is_active = window_id == active_window;

		push_parent(tab_widget);

		if (is_active) {
			const auto tab_highlight_widget = create_widget(std::format("{}_tab_highlight", window->name));

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

		const auto tab_name_widget = create_widget(std::format("{}_tab_name", window->name));

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
				.set<ImmediateId>({ tab_name_widget->id })
				.set<Text>(std::string(window->name))
				.set<TextFont>({
					.handle = ctx.world->get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
					.size = 24,
				})
				.set<TextColor>(WHITE);
		}

		tab_widget->entity.set<BackgroundColor>({ is_active ? Color::from_uint(40, 40, 40, 255) : Color::from_hex("#928374") });

		if (button_interaction(tab_widget)) {
			dock_node->active_window = window_id;
		}
		else if (drag_offset_interaction(tab_widget, glm::vec2(30.f, 30.f))) {
			ctx.drag_drop_ctx.state = DragAndDropContext::State::Dragging;

			push_parent(nullptr);

			const auto drag_tab_widget = create_widget("drab_tab_widget");

			if (is_new(*drag_tab_widget)) {
				drag_tab_widget->entity
					.add<Node>()
					.set<ImmediateId>({ drag_tab_widget->id })
					.set<BackgroundColor>(Color::from_uint(40, 40, 40, 255))
					.set<BorderColor>(Color::from_hex("#504945"))
					.set(GrowDirection::Horizontal)
					.set<SizeStrategy>({
						.x = Fixed{ 100.f },
						.y = Fixed { 28.f },
					});
			}

			drag_tab_widget->entity.set<Transform>({
				.translation = glm::vec3{ ctx.mouse_input.position, PREVIEW_WIDGET_LAYER },
			});

			pop_parent();
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
	const auto [docking_id, docking_pos, docking_size] = ps::visit(docking_target, ps::visitors{
		[](Window* window) {
			return std::tuple{ window->id, window->pos, window->size };
		},
		[](DockNode* node) {
			return std::tuple{ node->id, node->position, node->size };
		}
	});

	push_parent(nullptr);

	constexpr auto name_format = "dockspace_options_{}";
	const auto background_color = Color::from_uint(69, 133, 136, 180);
	const auto border_radius = 4.f;
	const auto size = 150.f;//std::min({ dock_node.size.x, dock_node.size.y, 150.f });

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

	if (is_new(*holder_widget)) {
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
			.set(SizeStrategy{
				.x = Fixed{ size },
				.y = Fixed{ size },
			})
			.set(GrowDirection::Vertical)
			.set<Node>({
				//.child_gap = { 4.f, 4.f },
			});

		top_row_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				//.child_gap = { 4.f, 4.f },
			});

		mid_row_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				//.child_gap = { 4.f, 4.f },
			});

		bot_row_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				//.child_gap = { 4.f, 4.f },
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
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = border_radius,
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
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = border_radius,
			});

		mid_mid_widget->entity
			.set(GrowDirection::Horizontal)
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = border_radius,
			});

		mid_right_widget->entity
			.set(GrowDirection::Horizontal)
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = border_radius,
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
			.set<BackgroundColor>(background_color)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.set<Node>({
				.border_radius = 4.f,
			});

		bot_right_widget->entity
			.set(GrowDirection::Horizontal)
			.set(SizeStrategy{
				.x = Grow{},
				.y = Grow{},
			})
			.add<Node>();
	}

	if (is_widget_hovered(*top_mid_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis::Horizontal;
		ctx.dock_ctx.payload.dock_side  = DockSide::TopLeft;
	}
	else if (is_widget_hovered(*bot_mid_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis::Horizontal;
		ctx.dock_ctx.payload.dock_side  = DockSide::BotRight;
	}
	else if (is_widget_hovered(*mid_left_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis::Vertical;
		ctx.dock_ctx.payload.dock_side  = DockSide::TopLeft;
	}
	else if (is_widget_hovered(*mid_right_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis::Vertical;
		ctx.dock_ctx.payload.dock_side  = DockSide::BotRight;
	}
	else if (is_widget_hovered(*mid_mid_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis::None;
		ctx.dock_ctx.payload.dock_side  = DockSide::Center;
	}
}

void Immediate::dock_outer_options(HashId dockspace_id) {
	const auto dockspace_node = get_dock_node(dockspace_id);

	if (!dockspace_node) {
		return;
	}

	push_parent(nullptr);

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
			ctx.dock_ctx.payload.dock_side = DockSide::Center;
			ctx.dock_ctx.payload.split_axis = SplitAxis::None;
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
		ctx.dock_ctx.payload.split_axis = SplitAxis::Horizontal;
		ctx.dock_ctx.payload.dock_side  = DockSide::TopLeft;
	}
	else if (is_widget_hovered(*bot_option_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis::Horizontal;
		ctx.dock_ctx.payload.dock_side  = DockSide::BotRight;
	}
	else if (is_widget_hovered(*left_option_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis::Vertical;
		ctx.dock_ctx.payload.dock_side  = DockSide::TopLeft;
	}
	else if (is_widget_hovered(*right_option_widget)) {
		ctx.dock_ctx.payload.split_axis = SplitAxis::Vertical;
		ctx.dock_ctx.payload.dock_side  = DockSide::BotRight;
	}
}

void Immediate::dock_preview(const DockTarget& docking_target, const DockTarget& docked_target, SplitAxis split_axis, DockSide dock_side) {
	const auto [docking_id, docking_pos, docking_size] = ps::visit(docking_target, ps::visitors{
		[](Window* window) {
			return std::tuple{ window->id, window->pos, window->size };
		},
		[](DockNode* node) {
			return std::tuple{ node->id, node->position, node->size };
		}
	});

	const auto docked_aspect = ps::visit(docked_target, ps::visitors{
		[](Window* window) {
			return 0.5f;
		},
		[](DockNode* node) {
			return node->aspect_ratio;
		}
	});

	push_parent(nullptr);

	const auto preview_widget = create_widget(std::format("dock_preview_{}", docking_id));

	const auto preview_color = Color::from_uint(69, 133, 136, 127);

	auto target_pos  = docking_pos;
	auto target_size = docking_size;

	if (split_axis != SplitAxis::None) {
		const auto side_coef = static_cast<float>(dock_side);

		target_size = split_axis == SplitAxis::Horizontal
			? glm::vec2{ docking_size.x, docking_size.y * docked_aspect }
			: glm::vec2{ docking_size.x * docked_aspect, docking_size.y };

		target_pos = split_axis == SplitAxis::Horizontal
			? glm::vec2{ docking_pos.x, docking_pos.y + target_size.y * side_coef }
			: glm::vec2{ docking_pos.x + target_size.x * side_coef, docking_pos.y };
	}

	const auto [edge_pos, edge_size] = [&]() -> std::pair<glm::vec2, glm::vec2> {
		if (dock_side == DockSide::Center) {
			return { docking_pos + docking_size * 0.5f, glm::vec2{ 0.f } };
		}

		const auto side_coef = static_cast<float>(dock_side);

		if (split_axis == SplitAxis::Horizontal) {
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

	constexpr auto pop_rate = 20.f;

	const auto pos_x  = animation_value(std::format("dock_preview_pos_x_{}",  docking_id), edge_pos.x,  target_pos.x,  pop_rate);
	const auto pos_y  = animation_value(std::format("dock_preview_pos_y_{}",  docking_id), edge_pos.y,  target_pos.y,  pop_rate);
	const auto size_x = animation_value(std::format("dock_preview_size_x_{}", docking_id), edge_size.x, target_size.x, pop_rate);
	const auto size_y = animation_value(std::format("dock_preview_size_y_{}", docking_id), edge_size.y, target_size.y, pop_rate);

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

Immediate::DockNode* Immediate::create_dock_child(DockNode* parent, DockSide side) {
	assert(parent != nullptr);

	auto node = DockNode{
		.parent_id = parent->id,
		.id = ctx.dock_ctx.next_id++,
	};

	parent->children[side == DockSide::TopLeft ? 0 : 1] = node.id;

	ctx.dock_ctx.node_id_to_node[node.id] = node;

	return &ctx.dock_ctx.node_id_to_node[node.id];
}

void Immediate::delete_dock_node(HashId dock_node_id) {
	ctx.dock_ctx.node_id_to_node.erase(dock_node_id);
}

std::pair<Immediate::DockNode*, Immediate::DockNode*> Immediate::split_node(HashId node_id, SplitAxis split_axis) {
	auto left = DockNode{
		.id = ctx.dock_ctx.next_id++,
	};
	auto right = DockNode{
		.id = ctx.dock_ctx.next_id++,
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

	ps::visit(docking_target, ps::visitors{
		[&](Window* window) {
			auto node =  create_dock_root(window->pos, window->size);

			ps::visit(docked_target, ps::visitors{
				[&](Window* docked_window) {
					const auto [left_node, right_node] = split_node(node->id, split_axis);
					const auto aspect_ratio = split_axis == SplitAxis::Horizontal
						? docked_window->size.y / window->size.y
						: docked_window->size.x / window->size.x;

					if (dock_side == DockSide::TopLeft) {
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
				},
				[&](DockNode* docked_node) {
					const auto docking_node = create_dock_child(node, dock_side == DockSide::TopLeft ? DockSide::BotRight : DockSide::TopLeft);

					docked_node->aspect_ratio = split_axis == SplitAxis::Horizontal
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
			ps::visit(docked_target, ps::visitors{
				[&](Window* window) {
					const auto [left_node, right_node] = split_node(node->id, split_axis);
					const auto aspect_ratio = split_axis == SplitAxis::Horizontal
						? window->size.y / node->size.y
						: window->size.x / node->size.x;

					if (dock_side == DockSide::TopLeft) {
						left_node->windows.push_back(window->id);
						left_node->active_window = window->id;
						left_node->aspect_ratio = std::min(0.5f, aspect_ratio);

						right_node->aspect_ratio = 1.f - left_node->aspect_ratio;

						if (node->central_node) {
							right_node->central_node = true;
							node->central_node = false;

							get_dock_root_node(node->id)->central_node_id = right_node->id;
						}

						window->dock_node_id = left_node->id;
					}
					else {
						right_node->windows.push_back(window->id);
						right_node->active_window = window->id;
						right_node->aspect_ratio = std::min(0.5f, aspect_ratio);

						left_node->aspect_ratio = 1.f - right_node->aspect_ratio;

						if (node->central_node) {
							left_node->central_node = true;
							node->central_node = false;

							get_dock_root_node(node->id)->central_node_id = left_node->id;
						}

						window->dock_node_id = right_node->id;
					}
				},
				[&](DockNode* node) {

				}
			});
		}
	});
	//const auto root_node = get_dock_root_node(docking_node->id);

	//if (dock_side == DockSide::Center) {

	//}
	//else {
	//	ps::visit(docked_target, ps::visitors{
	//		[&](Window* window) {
	//			const auto [left_node, right_node] = split_node(docking_node->id, split_axis);

	//			if (dock_side == DockSide::TopLeft) {
	//				left_node->windows.push_back(window->id);
	//				left_node->active_window = window->id;

	//				ps::visit(docking_target, ps::visitors{
	//					[&](Window* window) {
	//						right_node->windows.push_back(window->id);
	//						right_node->active_window = window->id;

	//						window->dock_node_id = right_node->id;
	//					},
	//					[&](DockNode* node) {
	//						
	//					}
	//				});

	//				window->dock_node_id = left_node->id;
	//			}
	//			else {
	//				right_node->windows.push_back(window->id);
	//				right_node->active_window = window->id;

	//				window->dock_node_id = right_node->id;
	//			}
	//		},
	//		[&](DockNode* node) {

	//		}
	//	});
	//	if (docking_node->central_node) {
	//		docking_node->central_node = false;

	//		const auto central_node = create_dock_child(docking_node, dock_side == DockSide::TopLeft ? DockSide::BotRight : DockSide::TopLeft);
	//		const auto docked_node = ps::visit(docked_target, ps::visitors{
	//			[&](Window* window) {
	//				auto node = create_dock_child(docking_node, dock_side);

	//				node->windows.push_back(window->id);
	//				node->active_window = window->id;

	//				return node;
	//			},
	//			[&](DockNode* node) {
	//				docking_node->children[static_cast<std::uint8_t>(dock_side)] = node->id;
	//				node->parent_id = docking_node->id;

	//				return node;
	//			}
	//		});

	//		docked_node->aspect_ratio = std::min(0.5f, split_axis == SplitAxis::Horizontal ? docked_node->size.y / docking_node->size.y : docked_node->size.x / docking_node->size.x);
	//		central_node->aspect_ratio = 1.f - docked_node->aspect_ratio;
	//		central_node->central_node = true;
	//		root_node->central_node_id = central_node->id;
	//	}
	//	else {
	//		if (root_node == docking_node) {
	//			auto [left_node, right_node] = split_node(docking_node->id, split_axis);
	//			auto docking_window = std::get<Window*>(docking_target);

	//			if (dock_side == DockSide::TopLeft) {
	//				right_node->windows.push_back(docking_window->id);
	//				right_node->active_window = docking_window->id;

	//				docking_window->dock_node_id = right_node->id;
	//			}
	//			else {
	//				left_node->windows.push_back(docking_window->id);
	//				left_node->active_window = docking_window->id;

	//				docking_window->dock_node_id = left_node->id;
	//			}
	//		}
	//		else {

	//		}
	//	}
	//}

	//auto root_node = [&] {
	//	if (ctx.id_to_window.contains(docking_id)) {
	//		const auto& docked_window = ctx.id_to_window.at(docking_id);

	//		if (!docked_window.dock_node_id) {
	//			return create_dock_root(docked_window.pos, docked_window.size);
	//		}

	//		return get_dock_node(docked_window.dock_node_id.value());
	//	}

	//	return get_dock_node(docking_id);
	//}();

	//if (dock_side == DockSide::Center) {
	//	if (root_node->windows.empty()) {
	//		root_node->windows.push_back(docked_window.id);
	//	}

	//	root_node->windows.push_back(docking_window.id);
	//	root_node->active_window = docking_window.id;

	//	for (auto& window_id : root_node->windows) {
	//		ctx.id_to_window.at(window_id).disabled = true;
	//	}

	//	docking_window.disabled = false;
	//	docking_window.dock_node_id = root_node->id;
	//	docked_window.dock_node_id = root_node->id;
	//}
	//else if (root_node->central_node_id != 0) {
	//	auto [left, right] = split_node(root_node->central_node_id, split_axis);

	//	get_dock_node(root_node->central_node_id)->central_node = false;

	//	if (dock_side == DockSide::TopLeft) {
	//		left->windows.push_back(docked_window.id);
	//		left->active_window = docked_window.id;

	//		docking_window.dock_node_id = left->id;

	//		root_node->central_node_id = right->id;
	//	}
	//	else {
	//		right->windows.push_back(docked_window.id);
	//		right->active_window = docked_window.id;

	//		docking_window.dock_node_id = right->id;

	//		root_node->central_node_id = left->id;
	//	}

	//	get_dock_node(root_node->central_node_id)->central_node = true;
	//}
	//else {
	//	auto [left, right] = split_node(root_node->id, split_axis);

	//	docking_window.dock_node_id = dock_side == DockSide::TopLeft ? left->id : right->id;
	//	docked_window.dock_node_id = dock_side == DockSide::TopLeft ? right->id : left->id;

	//	left->windows.push_back(dock_side == DockSide::TopLeft ? docking_window.id : docked_window.id);
	//	right->windows.push_back(dock_side == DockSide::TopLeft ? docked_window.id : docking_window.id);

	//	left->active_window = dock_side == DockSide::TopLeft ? docking_window.id : docked_window.id;
	//	right->active_window = dock_side == DockSide::TopLeft ? docked_window.id : docking_window.id;

	//	left->widget_id = dock_side == DockSide::TopLeft ? docking_window.id : docked_window.id;
	//	right->widget_id = dock_side == DockSide::TopLeft ? docked_window.id : docking_window.id;
	//}
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

	auto window_it = std::ranges::find(dock_node->windows, window.id);

	if (window_it != dock_node->windows.end()) {
		dock_node->windows.erase(window_it);
	}

	if (!dock_node->windows.empty()) {
		dock_node->active_window = dock_node->windows.back();
	}
}

void Immediate::update_dock_nodes() {
	std::vector<HashId> nodes_to_delete;

	for (auto node_id : ctx.dock_ctx.roots) {
		bfs(node_id, [&](DockNode* node) {
			const auto leaf_node = node->children[0] == 0;

			if (leaf_node && node->windows.empty() && node->central_node_id == 0 && !node->central_node) {
				nodes_to_delete.push_back(node->id);

				return;
			}

			if (leaf_node) {
				if (node->parent_id == 0 && node->windows.size() == 1) {
					nodes_to_delete.push_back(node->id);
				}
				else if (!node->windows.empty()) {
					for (auto window : node->windows) {
						ctx.id_to_window.at(window).disabled = true;
					}

					ctx.id_to_window.at(node->active_window).disabled = false;
				}
			}
			else {
				for (size_t i = 0; i < node->children.size(); ++i) {
					const auto child_id = node->children[i];

					const auto multiplier = static_cast<float>(i);
					const auto child = get_dock_node(child_id);

					child->position = node->position;

					if (node->split_axis == SplitAxis::Horizontal) {
						child->size.x = node->size.x;
						child->size.y = node->size.y * child->aspect_ratio;
						child->position.y += node->size.y * multiplier - child->size.y * multiplier;
					}
					else {
						child->size.x = node->size.x * child->aspect_ratio;
						child->size.y = node->size.y;
						child->position.x += node->size.x * multiplier - child->size.x * multiplier;
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

				parent->split_axis = SplitAxis::None;
				parent->windows = other_child->windows;
				parent->active_window = other_child->active_window;

				ctx.dock_ctx.node_id_to_node.erase(other_child->id);
			}
			else {
				const auto other_child = get_dock_node(parent->children[0]);

				parent->split_axis = SplitAxis::None;
				parent->windows = other_child->windows;
				parent->active_window = other_child->active_window;

				ctx.dock_ctx.node_id_to_node.erase(other_child->id);
			}

			for (const auto window_id : parent->windows) {
				if (ctx.id_to_window.contains(window_id)) {
					ctx.id_to_window.at(window_id).dock_node_id = parent->id;
				}
			}

			parent->children[0] = 0;
			parent->children[1] = 0;
		}
		else {
			for (const auto window_id : node->windows) {
				if (ctx.id_to_window.contains(window_id)) {
					auto& window = ctx.id_to_window.at(window_id);

					window.dock_node_id.reset();
					window.disabled = false;
				}
			}
		}

		ctx.dock_ctx.node_id_to_node.erase(id);
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
