#pragma once

#include "SFML/Graphics.hpp"

namespace ps::ecsModule {
	struct RendererComponent {
		std::unique_ptr<sf::RenderWindow> renderer;
	};
}
