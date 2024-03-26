#pragma once

#include "SFML/System/Clock.hpp"

namespace ps::ecsModule {
	struct TimeComponent {
		float deltaTime = 0.f;
		sf::Clock deltaClock;
	};
}
