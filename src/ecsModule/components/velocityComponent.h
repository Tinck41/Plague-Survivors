#pragma once

#include "SFML/Graphics.hpp"

namespace ps::ecsModule {
	struct VelocityComponent {
		sf::Vector2f velocity{ 0.f, 0.f };
	};
}

