#pragma once

#include "core/gameObject.h"
#include "SFML/Graphics.hpp"

namespace ps::core {
	class SceneBase {
	public:
		SceneBase() = default;
		virtual ~SceneBase() = default;

		virtual void update(float delta) {}
		virtual void render(sf::RenderWindow* window) {}
	private:
		std::unique_ptr<GameObject> m_root;
	};
}
