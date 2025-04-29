#pragma once

#include "SFML/Graphics/RectangleShape.hpp"
#include "flecs.h"

namespace ps {
	//struct Sprite {
	//	sf::RectangleShape sprite;
	//	sf::Color color;
	//};

	struct SpriteModule {
		SpriteModule(flecs::world& world);
	};
}
