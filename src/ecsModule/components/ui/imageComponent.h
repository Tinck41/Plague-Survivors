#pragma once

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Texture.hpp"
#include <string>

namespace ps::ecsModule::ui {
	struct ImageComponent {
		sf::Texture texture;
		sf::Color color;
		std::string path;
	};
}
