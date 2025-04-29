#pragma once

#include "SFML/System/Vector2.hpp"
#include "flecs.h"

namespace ps {
	struct Player {};

	struct IdleState {};
	struct HandleInputState {};
	struct DashState {};

	struct Dash {
		sf::Vector2f direction = { 0.f, 0.f };
		float duration = 0.f;
		float distance = 0.f;
		float timer = 0.f;
	};

	struct PlayerModule {
		PlayerModule(flecs::world& world);
	};
}
