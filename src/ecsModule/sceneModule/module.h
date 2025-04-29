#pragma once

#include "flecs.h"

namespace ps {
	using Pipeline = flecs::entity;

	struct ActiveScene{};
	struct SceneRoot{};

	struct MenuScene{
		Pipeline pip;
	};

	struct GameScene{
		Pipeline pip;
	};

	struct SceneModule {
		SceneModule(flecs::world& world);
	};
}
