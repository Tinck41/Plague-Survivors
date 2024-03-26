#pragma once

#include "SFML/Graphics.hpp"

namespace ps::ecsModule {
	struct TransformComponent {
		sf::Vector2f trasnslation{ 0.f, 0.f };
		sf::Vector2f scale{ 1.f, 1.f };
		float rotation = 0.f;
	};
}
