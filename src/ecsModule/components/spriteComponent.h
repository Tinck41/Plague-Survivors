#pragma once

#include "SFML/Graphics.hpp"

namespace ps::ecsModule {
	struct SpriteComponent {
		sf::RectangleShape sprite;
		sf::Color color;
	};
}

