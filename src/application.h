#pragma once

#include "SFML/Graphics/RenderWindow.hpp"
#include "core/sceneController.h"

#include <memory>

namespace ps::core {
	class Application {
	public:
		Application();

		void init();
		void run();
		void shutdown();
	private:
		void processInput();
		void update();
		void render();

		std::unique_ptr<sf::RenderWindow> m_window;
		std::unique_ptr<SceneController> m_sceneController;
	};
}
