#pragma once

#include "flecs.h"

namespace se {
	struct FpsLabel {};
	struct DrawCallsLabel {};

	struct FpsUpdateTimer {
		float elapsed_time;
		float update_rate;
	};

	struct DebugModule {
		DebugModule(flecs::world& world);
	};
}
