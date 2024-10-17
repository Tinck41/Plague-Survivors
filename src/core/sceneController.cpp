#include "sceneController.h"
#include "core/sceneBase.h"

using namespace ps::core;

SceneBase* SceneController::getCurrentScene() const {
	return m_currentScene.get();
}

void SceneController::setCurrentScene(std::unique_ptr<SceneBase> newScene) {
	m_currentScene.reset();
	m_currentScene = std::move(newScene);
}

void SceneController::update(float delta) {
	m_currentScene->update(delta);
}

void SceneController::render(sf::RenderWindow* window) {
	m_currentScene->render(window);
}
