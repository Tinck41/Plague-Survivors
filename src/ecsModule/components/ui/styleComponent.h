#pragma once

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/System/Vector2.hpp"

namespace ps::ecsModule::ui {
	struct StyleComponent {
		sf::Vector2f size { 0.f, 0.f };
		sf::Color backgroundColor{ sf::Color::Transparent };
		sf::RectangleShape shape;
	};
}
