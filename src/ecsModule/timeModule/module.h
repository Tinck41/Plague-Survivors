#pragma once

#include "flecs.h"

namespace ps {
	struct Time {
		float deltaTime;
	};

	struct TimeModule {
		TimeModule(flecs::world& world);
	};
}
