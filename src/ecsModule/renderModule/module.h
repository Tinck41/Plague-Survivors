#pragma once

#include "SFML/Graphics/CircleShape.hpp"
#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/RenderWindow.hpp"
#include "SFML/Graphics/Shape.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "ecsModule/transformModule/module.h"
#include "flecs.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace ps {
	struct RenderItem {
		sf::Drawable* item;
		sf::RenderStates states = sf::RenderStates::Default;
	};

	//struct RenderQueue {
	//	std::vector<sf::Drawable*> drawables;
	//};
	
	using RenderQueue = std::multimap<int, sf::Drawable*>;

	struct Sprite {
		std::shared_ptr<sf::Texture> texture;
		sf::Color color = sf::Color::White;
		std::optional<Vec2f> size;
		sf::Sprite* sprite;
		sf::RenderStates states = sf::RenderStates::Default;
	};

	using RenderFunc = std::function<void(sf::RenderWindow& window, flecs::entity e)>;

	struct Rectangle {
		sf::Vector2f size;
	};

	struct Circle {
		float radius;
	};

	//using Rectangle = sf::RectangleShape;
	//using Circle = sf::CircleShape;

	struct RenderModule {
		RenderModule(flecs::world& world);
	};
}
