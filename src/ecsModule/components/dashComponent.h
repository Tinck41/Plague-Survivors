#pragma once

#include "SFML/System/Vector2.hpp"

namespace ps::ecsModule {
	struct DashComponent {
		sf::Vector2f direction = { 0.f, 0.f };
		float duration = 0.f;
		float distance = 0.f;
		float timer = 0.f;
	};
}
