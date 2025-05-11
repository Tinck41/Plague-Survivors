#pragma once

#include "flecs.h"
#include "vec2.hpp"

namespace ps {
	struct Player {};

	enum class ePlayerState {
		Idle,
		Moving,
		Dashing,
	};

	struct CameraTarget {};

	struct DashData {
		float distance;
		float duration;

		float timer;

		glm::vec2 startVelocity;
	};

	struct PlayerModule {
		PlayerModule(flecs::world& world);
	};
}
