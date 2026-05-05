#pragma once

#include "ecsModule/ui_module/module.h"
#include "glm.hpp"
#include "flecs.h"

#include <array>
#include <functional>
#include <unordered_map>
#include <vector>

namespace ps {
	using DockNodeId = std::uint32_t;

	enum class SplitAxis : std::uint8_t {
		None,
		Horizontal,
		Vertical
	};

	enum class DockSide : std::uint8_t {
		TopLeft = 0,
		BotRight = 1,
	};

	struct DockNode {
		flecs::entity preview_entity;
		flecs::entity options_entity;

		std::vector<flecs::entity> windows;
		size_t active_window = 0;

		DockNodeId parent_id = 0;
		DockNodeId id = 0;
		DockNodeId central_node_id = 0;
		std::array<DockNodeId, 2> children;

		glm::vec2 size;
		glm::vec2 position;

		float aspect_ratio = 1.f;

		bool dockspace = false;

		SplitAxis split_axis;
	};

	struct DockNodeRef {
		DockNodeId id;
	};

	struct DockingSpace {};
	struct DockSpaceNode {};
	struct DockingOption {};
	struct DockPreview {};
	struct DockOptions {};
	struct DockOptionsNode {};
	struct DockPreviewNode {};
	struct DockingEnabled {};

	struct WindowTarget {
		flecs::entity window;
		glm::vec2 size;
		glm::vec2 position;
	};

	struct NodeTarget {
		DockNodeId id;
	};

	using DockingTarget = std::variant<WindowTarget, NodeTarget>;

	struct DockData {
		DockingTarget target;
		SplitAxis split_axis;
		DockSide dock_side;

		flecs::entity dock_options_container;
	};

	class DockTree {
	public:
		void add_dockspace(flecs::entity& dockspace);
		void dock_window(flecs::entity source, const Node& soruce_node, const DockingTarget& target, SplitAxis split_axis, DockSide dock_side);
		void undock_window(flecs::entity window, DockNodeId node_id);

		void bfs(DockNodeId node_id, const std::function<void(DockNode&)>& callback);
		void dfs(DockNodeId node_id, const std::function<void(DockNode&)>& callback);
		void reverse_dfs(DockNodeId node_id, const std::function<void(DockNode&)>& callback);

		void update();
		void cleanup();

		DockNode& get_node(DockNodeId node_id);
		DockNode& get_root_node(DockNodeId node_id);

		const std::vector<DockNodeId>& get_root_nodes() const;

		bool destroy = true;

	private:
		void remove_child(DockNodeId parent, DockNodeId child);
		void update_buttons(DockNode& root);

		bool has_children(DockNodeId node_id);

		DockNode& create_root(glm::vec2 position, glm::vec2 size);

		std::pair<DockNode&, DockNode&> split_node(DockNodeId node_id, SplitAxis axis);

		std::unordered_map<DockNodeId, DockNode> nodes;
		std::vector<DockNodeId> root_node_ids;
		std::vector<DockNodeId> remove_ids;

		DockNodeId next_id = 0;
	};

	struct DockingModule {
		DockingModule(flecs::world& world);
	};

	flecs::entity create_dockspace(flecs::world& world, flecs::entity window, const std::string& name);
	flecs::entity create_dockspace_preview(flecs::world& world, glm::vec2 size, SplitAxis split_axis, DockSide dock_side);
	flecs::entity create_dockspace_inner_options(flecs::world& world, const DockNode& dock_node);
	flecs::entity create_dockspace_outer_options(flecs::world& world, const DockNode& dock_node);
}
