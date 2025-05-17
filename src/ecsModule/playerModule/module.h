#pragma once

#include "flecs.h"
#include "vec2.hpp"
#include <unordered_map>

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

	struct CameraShakeData {
		enum class State {
			Increasing,
			Stagnating,
			Decreasing,
		};

		struct StateData {
			float duration;
			float timer;
		};

		float amplitude;

		State currentState;

		std::unordered_map<State, StateData> stateData;
	};

	struct PlayerModule {
		PlayerModule(flecs::world& world);
	};
}
