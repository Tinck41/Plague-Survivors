#pragma once

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
		using WidgetId = std::uint64_t;

		struct Window {
			enum Flags {
				None       = 1 << 0,
				NoTitlebar = 1 << 1,
				NoMove     = 1 << 2,
				NoClose    = 1 << 2,
				NoCollapse = 1 << 3,
				NoResize   = 1 << 4,
				NoDocking  = 1 << 5,
				Size       = 1 << 6,
			};

			WidgetId id;

			bool has_focus = true;
			bool collapsed = false;
			bool disabled = false;

			flecs::entity_t entity;

			std::string name;
			std::optional<HashId> dock_node_id;

			glm::vec2 size { 200.f, 200.f };
			glm::vec2 pos  { 0.f, 0.f };

			Flags flags;
		};

		enum class SplitAxis : std::uint8_t {
			None,
			Horizontal,
			Vertical
		};

		enum class DockSide : std::uint8_t {
			TopLeft = 0,
			BotRight = 1,
			Center = 2,
			None,
		};

		struct DockNode {
			std::vector<HashId> windows;

			HashId active_window = 0;

			WidgetId widget_id = 0;

			HashId central_node_id = 0;
			HashId parent_id       = 0;
			HashId id              = 0;

			std::array<HashId, 2> children;

			glm::vec2 size;
			glm::vec2 position;

			float aspect_ratio = 1.f;

			bool dockspace = false;
			bool central_node = false;
			bool root = false;

			SplitAxis split_axis;

			bool is_leaf() { return children[0] == 0; };
		};

		struct AnimNode {
			std::uint64_t last_active_frame  = 0;
			std::uint64_t first_active_frame = 0;

			float initial;
			float current;
			float target;
			float rate;
		};

		enum WidgetFlags {
			None      = 0 << 0,
			Clickable = 1 << 0,
		};

		struct Widget {
			bool operator ==(const Widget& other) {
				if (&other == this) {
					return true;
				}

				return other.id == id;
			}

			flecs::entity entity = flecs::entity::null();

			Widget* first  = nullptr;
			Widget* last   = nullptr;
			Widget* next   = nullptr;
			Widget* prev   = nullptr;
			Widget* parent = nullptr;

			glm::vec4 last_rect;

			HashId id = 0;

			std::uint64_t last_active_frame  = 0;
			std::uint64_t first_active_frame = 0;

			WidgetFlags flags = WidgetFlags::None;
		};

		using DockTarget = std::variant<Window*, DockNode*>;

		struct DockContext {
			struct Payload {
				SplitAxis split_axis = SplitAxis::None;
				DockSide  dock_side  = DockSide::None;

				std::optional<DockTarget> docking_target;
				std::optional<DockTarget> docked_target;
			};

			std::unordered_map<WidgetId, HashId> widget_id_to_node_id;
			std::unordered_map<HashId, DockNode> node_id_to_node;

			std::vector<HashId> roots;

			HashId next_id = 1;
			Payload payload;
		};

		struct DragAndDropContext {
			enum class State {
				None,
				Dragging,
				Dropping,
				Size,
			};

			State state           = State::None;

			glm::vec2 start_pos   = {};
			glm::vec2 current_pos = {};
		};

		struct Context {
			Window* active_window;

			std::vector<Window*> windows;
			std::vector<Widget*> widgets;
			std::vector<Widget*> roots;

			std::unordered_map<HashId, Widget>   id_to_widget;
			std::unordered_map<HashId, AnimNode> id_to_anim;
			std::unordered_map<HashId, Window>   id_to_window;

			std::stack<Widget*> parent_stack;

			Widget* hot_widget;                                               // The hovered widget
			Widget* active_widget;                                            // The clicked widget

			std::optional<Widget*> active_drag;

			flecs::entity root;

			flecs::query<> update_query;
			flecs::query<> cleanup_query;
			flecs::world* world;

			MouseState mouse_input;

			DockContext dock_ctx;
			DragAndDropContext drag_drop_ctx;

			size_t widget_num = 0;

			size_t frame_count = 0;
		};

		void init(flecs::world& world);

		void push_style();
		void pop_style();

		void begin(std::string name, int flags = {}, std::optional<glm::vec2> pos = std::nullopt, std::optional<glm::vec2> size = std::nullopt);
		void end();

		void dockspace();

		bool button(std::string name);

		void text(std::string text);

		void begin_frame();
		void end_frame();

		float animation_value(std::string name, float initial, float target, float rate);
		float animation_precent(std::string name, float initial, float target, float rate);

		bool clear_on_frame_end = true;
	private:
		HashId hash(std::string_view name);

		Widget* get_widget_by_id(HashId id);
		Widget* create_widget(std::string_view name, WidgetFlags flags = WidgetFlags::None);

		void insert_widget_in_tree(Widget* widget, Widget* parent);

		void children(Widget* parent, const std::function<void(Widget*)>& callback);

		void dfs(Widget* root, const std::function<void(Widget*)>& callback);
		void bfs(std::span<Widget*> roots, const std::function<void(Widget*)>& callback);

		void calculate_dfs_indices();
		void calculate_bfs_indices();

		void clear_inactive_widgets();
		void clear_inactive_animations();

		void process_drag_drop();

		void push_parent(HashId widget_id);
		void push_parent(Widget* widget);
		void pop_parent();

		Widget* get_parent();

		bool is_widget_pressed(const Widget& widget);
		bool is_widget_released(const Widget& widget);
		bool is_widget_down(const Widget& widget);
		bool is_widget_hovered(const Widget& widget);

		bool drag_interaction(Widget* widget);
		bool drag_offset_interaction(Widget* widget, glm::vec2 offset);
		bool button_interaction(Widget* widget);

		bool is_collide_with(HashId left, HashId right);
		bool is_mouse_collide_with(HashId widget_id);

		bool is_new(const Widget& widget);

		void titlebar(Window& window);
		void tabsbar(Window& window);

		void dock_inner_options(const DockTarget& docking_target);
		void dock_outer_options(HashId dockspace_id);
		void dock_preview(const DockTarget& docking_target, const DockTarget& docked_target, SplitAxis split_axis, DockSide dock_side);

		DockNode* get_dock_node(HashId node_id);
		DockNode* get_dock_root_node(HashId node_id);
		DockNode* create_dock_root(glm::vec2 position, glm::vec2 size);
		DockNode* create_dock_child(DockNode* parent, DockSide side);

		void delete_dock_node(HashId node_id);

		std::pair<DockNode*, DockNode*> split_node(HashId node_id, SplitAxis split_axis);

		void apply_dock(const DockTarget& docking_target, const DockTarget& docked_target, SplitAxis split_axis, DockSide dock_side);
		void undock_window(Window& window);

		void update_dock_nodes();

		void bfs(HashId root, const std::function<void(DockNode*)>& callback);

		Context ctx;
		Context last_ctx;
	};

	struct ImmediateId {
		Immediate::WidgetId value;
	};
}
