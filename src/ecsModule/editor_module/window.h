#pragma once

#include "ext/vector_float2.hpp"
#include "flecs.h"

#include <optional>
#include <string>

namespace ps {
	enum WindowFlags : std::uint8_t {
		None = 0,
		NoCollapse = 1 << 0,
		NoMove = 1 << 1,
		NoResize = 1 << 2,
		NoScrollbar = 1 << 3,
		NoClose = 1 << 4,
		NoTitlebar = 1 << 5,
		NoDocking = 1 << 6,
	};

	struct WindowConfig {
		std::string name;
		std::uint8_t flags = 0;
		std::optional<glm::vec2> size;
	};

	struct EditorWindow {
		flecs::entity_t content;

		std::uint8_t flags = 0;

		glm::vec2 size;

		bool collapsed = false;
		bool docked = false;
	};

	struct WindowContent {};
	struct WindowTitlebar {};
	struct WindowTab {};
	struct CollapseButton {};
	struct WindowCollapseButton {};
	struct WindowResizeButton {};

	struct CollapseTarget {};
	struct ResizeTarget {};
	struct CloseTarget {};
	struct DragTarget {};

	struct NowDragged {};

	struct TrackCursor {
		glm::vec2 offset;
	};

	std::pair<flecs::entity, flecs::entity> create_window(flecs::world& world, WindowConfig config);
}
