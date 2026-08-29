#pragma once

#include "SDL3/SDL_mouse.h"
#include "ecsModule/inputModule/module.h"
#include "glm.hpp"
#include "flecs.h"

#include <functional>
#include <optional>
#include <span>
#include <stack>
#include <string>
#include <variant>
#include <vector>
#include <unordered_set>

namespace se {
	class MouseState;
}

namespace se::editor {
	class Immediate {
	public:
		using HashId = std::uint64_t;

		using WindowFlags = int;
		using WidgetFlags = int;

		enum WindowFlags_ {
			WindowFlags_None        = 0,
			WindowFlags_NoTitlebar  = 1 << 1,
			WindowFlags_NoMove      = 1 << 2,
			WindowFlags_NoClose     = 1 << 2,
			WindowFlags_NoCollasee  = 1 << 3,
			WindowFlags_NoResize    = 1 << 4,
			WindowFlags_NoDocking   = 1 << 5,
			WindowFlags_FitViewport = 1 << 6,
			WindowFlags_FixedFocus  = 1 << 7,
			WindowFlags_Size,
		};

		enum Direction {
			Direction_None  = 0,
			Direction_Left  = 1,
			Direction_Up    = 2,
			Direction_Right = 3,
			Direction_Down  = 4,
			Direction_Size,
		};

		enum SplitAxis : std::uint8_t {
			SplitAxis_None,
			SplitAxis_Horizontal,
			SplitAxis_Vertical
		};

		enum DockSide : std::uint8_t {
			DockSide_TopLeft = 0,
			DockSide_BotRight = 1,
			DockSide_Center = 2,
			DockSide_None,
		};

		enum WidgetFlags_ {
			WidgetFlags_None      = 0 << 0,
			WidgetFlags_Clickable = 1 << 0,
		};

		enum DragAndDropState {
			DragAndDropState_None,
			DragAndDropState_Dragging,
			DragAndDropState_Dropping,
			DragAndDropState_Size,
		};

		struct Window {
			HashId                id;
			HashId                overlay_id;

			bool                  has_focus         = false;
			bool                  colapsed          = false;
			bool                  disabled          = false;

			flecs::entity_t       entity;

			size_t                focus_order       = 0;

			std::string           name;
			std::optional<HashId> dock_node_id;
			std::optional<HashId> dock_node_host_id;

			glm::vec2            size;
			glm::vec2            pos;

			WindowFlags          flags;
		};

		struct DockNode {
			std::vector<HashId>   windows;

			HashId                active_window     = 0;
			HashId                host_window       = 0;

			HashId                widget_id         = 0;
			HashId                overlay_widget_id = 0;

			HashId                central_node_id   = 0;
			HashId                parent_id         = 0;
			HashId                id                = 0;

			std::array<HashId, 2> children;

			glm::vec2             size;
			glm::vec2             position;

			size_t                focus_order      = 0;

			float                 aspect_ratio     = 1.f;

			bool                  dockspace        = false;
			bool                  central_node     = false;
			bool                  root             = false;

			SplitAxis             split_axis;

			bool is_leaf() { return children[0] == 0; };
		};

		struct AnimNode {
			std::uint64_t last_active_frame  = 0;
			std::uint64_t first_active_frame = 0;

			float         initial;
			float         current;
			float         target;
			float         rate;
		};

		struct Widget {
			flecs::entity entity             = flecs::entity::null();

			Widget*       first              = nullptr;
			Widget*       last               = nullptr;
			Widget*       next               = nullptr;
			Widget*       prev               = nullptr;
			Widget*       parent             = nullptr;

			glm::vec4     last_rect;

			HashId        id                 = 0;

			std::uint64_t last_active_frame  = 0;
			std::uint64_t first_active_frame = 0;

			WidgetFlags   flags;

			bool operator ==(const Widget& other) {
				if (&other == this) {
					return true;
				}

				return other.id == id;
			}
		};

		struct WidgetGroup {
			std::vector<HashId> widgets;

			size_t sort_value;
		};

		using DockTarget = std::variant<Window*, DockNode*>;

		struct DockPayload {
			SplitAxis                 split_axis     = SplitAxis_None;
			DockSide                  dock_side      = DockSide_None;

			std::optional<DockTarget> docking_target;
			std::optional<DockTarget> docked_target;
		};

		struct DockContext {
			std::unordered_map<HashId, HashId>   widget_id_to_node_id;
			std::unordered_map<HashId, DockNode> node_id_to_node;

			std::unordered_set<HashId>           fresh_docks; // TODO: need for prevent dock nodes that were just created didn't brake draw order, otherwise window with this id will be skipped.
			std::vector<HashId>                  roots;

			HashId                               next_id = 1;
			DockPayload                          payload;

			float                                node_gap = 2.f;
		};

		struct DragAndDropContext {
			DragAndDropState state        = DragAndDropState_None;

			glm::vec2        start_pos    = {};
			glm::vec2        current_pos  = {};
		};

		struct Context {
			Window*                              current_window;

			std::vector<Window*>                 windows_focus_order;
			std::vector<Widget*>                 roots;

			std::unordered_map<HashId, Widget>   id_to_widget;
			std::unordered_map<HashId, AnimNode> id_to_anim;
			std::unordered_map<HashId, Window>   id_to_window;

			std::stack<Widget*>                  parent_stack;

			Widget*                              hot_widget             = nullptr;                                           // The hovered widget
			Widget*                              active_widget          = nullptr;                                           // The clicked widget
			Widget*                              general_overlay_widget = nullptr;

			flecs::entity                        root;

			flecs::query<>                       update_query;
			flecs::world*                        world;

			MouseState                           mouse_input;

			DockContext                          dock_ctx;
			DragAndDropContext                   drag_drop_ctx;

			SDL_SystemCursor                     cursor_type          = SDL_SYSTEM_CURSOR_DEFAULT;
			SDL_SystemCursor                     last_cursor_type     = SDL_SYSTEM_CURSOR_DEFAULT;
			SDL_Cursor*                          cursor               = nullptr;

			size_t                               frame_count          = 0;
		};

		void                            init(flecs::world& world);

		void                            push_style();
		void                            pop_style();

		void                            begin(std::string name, WindowFlags flags = {}, std::optional<glm::vec2> pos = std::nullopt, std::optional<glm::vec2> size = std::nullopt);
		void                            end();

		void                            dockspace(const std::string& name, glm::vec2 size);

		bool                            button(std::string name);

		void                            text(std::string text);

		void                            begin_frame();
		void                            end_frame();

		float                           animation_value(std::string name, float initial, float target, float rate);
		float                           animation_precent(std::string name, float initial, float target, float rate);

		HashId                          hash(std::string_view name);

		bool clear_on_frame_end = true;
	private:

		// General
		Widget*                         get_widget_by_id(HashId id);
		Widget*                         create_widget(std::string_view name, WidgetFlags flags = WidgetFlags_None);

		void                            insert_widget_in_tree(Widget* widget, Widget* parent);

		void                            children(Widget* parent, const std::function<void(Widget*)>& callback);

		void                            dfs(Widget* root, const std::function<void(Widget*)>& callback);
		void                            bfs(std::span<Widget*> roots, const std::function<void(Widget*)>& callback);

		std::vector<HashId>             compose_render_queue();

		void                            calculate_dfs_indices(const std::vector<HashId>& render_queue);
		void                            calculate_bfs_indices(const std::vector<HashId>& render_queue);

		void                            clear_inactive_widgets();
		void                            clear_inactive_animations();

		void                            process_drag_drop();

		void                            push_parent(HashId widget_id);
		void                            push_parent(Widget* widget);
		void                            pop_parent();

		Widget*                         get_parent();

		// Interaction
		bool                            is_widget_hovered(const Widget& widget);

		bool                            drag_interaction(Widget* widget);
		bool                            drag_offset_interaction(Widget* widget, glm::vec2 offset);
		bool                            button_interaction(Widget* widget);
		bool                            hover_interaction(Widget* widget);

		bool                            is_collide_with(HashId left, HashId right);
		bool                            is_mouse_collide_with(HashId widget_id);

		bool                            is_new(const Widget& widget);

		void                            titlebar(Window& window);
		void                            tabsbar(Window& window);

		// Docking
		void                            dock_inner_options(const DockTarget& docking_target);
		void                            dock_outer_options(HashId dockspace_id);
		void                            dock_preview(const DockTarget& docking_target, const DockTarget& docked_target, SplitAxis split_axis, DockSide dock_side);

		DockNode*                       get_dock_node(HashId node_id);
		DockNode*                       get_dock_root_node(HashId node_id);
		DockNode*                       create_dock_root(glm::vec2 position, glm::vec2 size);
		DockNode*                       create_dock_root(HashId id, glm::vec2 position, glm::vec2 size);
		DockNode*                       create_dock_child(DockNode* parent, DockSide side);

		std::vector<HashId>             get_all_node_windows(DockNode* root);

		void                            delete_dock_node(HashId node_id);
		void                            dock_node_update_ratio(HashId node_id, float new_ratio);

		std::pair<DockNode*, DockNode*> split_node(HashId node_id, SplitAxis split_axis);

		void                            apply_dock(const DockTarget& docking_target, const DockTarget& docked_target, SplitAxis split_axis, DockSide dock_side);
		void                            undock_window(Window& window);

		void                            update_dock_nodes();

		void                            bfs(HashId root, const std::function<void(DockNode*)>& callback);

		void                            focus_window(Window& window);
		void                            grip_window_borders(Window& window);
		void                            grip_dock_borders(DockNode& node);

		Context ctx;
		Context last_ctx;
	};

	struct ImmediateId {
		Immediate::HashId value;
	};
}
