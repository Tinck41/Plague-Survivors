#pragma once

#include "SFML/Graphics/RenderWindow.hpp"
#include "core/sceneBase.h"
#include <memory>

namespace ps::core {
	class SceneController {
	public:
		SceneController() = default;

		SceneBase* getCurrentScene() const;

		void setCurrentScene(std::unique_ptr<SceneBase> newScene);

		void update(float delta);
		void render(sf::RenderWindow* window);
	private:
		std::unique_ptr<SceneBase> m_currentScene;
	};
}
