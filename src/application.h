#pragma once

#include <memory>
#include "SFML/Graphics/RenderWindow.hpp"

namespace ps::core {
	class Application {
	public:
		[[nodiscard]] static std::unique_ptr<Application> create();

		void init();
		void run();
		void shutdown();
	private:
		Application() = default;

		void processInput();
		void update();
		void render();

		std::unique_ptr<sf::RenderWindow> m_window;
	};
}
