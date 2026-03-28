#pragma once

#include "flecs.h"

namespace ps {
	struct FpsLabel {};
	struct DrawCallsLabel {};

	struct DebugModule {
		DebugModule(flecs::world& world);
	};
}
