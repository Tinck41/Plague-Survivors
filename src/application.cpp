#include "application.h"

#include "SFML/Graphics.hpp"
#include "core/gameObject.h"
#include "core/sceneController.h"

using namespace ps;
using namespace core;

Application::Application() {
	m_window          = std::make_unique<sf::RenderWindow>();
	m_sceneController = std::make_unique<SceneController>();
}

void Application::init() {
}

void Application::run() {
	sf::Clock deltaClock;

	while (m_window->isOpen()) {
		m_sceneController->update(deltaClock.restart().asSeconds());
		m_sceneController->render(m_window.get());
	}
}

void Application::shutdown() {
}
