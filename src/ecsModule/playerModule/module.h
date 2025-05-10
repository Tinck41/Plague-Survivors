#pragma once

#include "flecs.h"

namespace ps {
	struct Player {};

	struct PlayerModule {
		PlayerModule(flecs::world& world);
	};
}
