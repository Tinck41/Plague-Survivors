#pragma once

#include "SFML/Graphics.hpp"
#include "flecs.h"

namespace ps {
	struct Application {
		sf::RenderWindow window;
	};

	struct AppModule {
		AppModule(flecs::world& world);
	};
}
