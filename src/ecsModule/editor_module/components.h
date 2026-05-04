#pragma once

#include "flecs.h"
#include "palette.h"

namespace ps {
	struct EditorSettings {
		Palette palette;
	};

	struct EditorNode {};

	struct Viewport {};
	struct Inspector {};
	struct Console {};

	struct Hierarchy {
		flecs::query<> query;
	};

	struct TreeNode {
		flecs::entity_t entity;
		flecs::entity_t container;
		flecs::entity_t self_container;

		bool selected = false;
	};

	struct TreeNodePart {};
	struct TreeNodeSelected {};
}
