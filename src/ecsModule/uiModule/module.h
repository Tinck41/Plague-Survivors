#pragma once

#include "SFML/Graphics/Color.hpp"
#include "SFML/Graphics/Image.hpp"
#include "SFML/Graphics/RectangleShape.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Text.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/System/Vector2.hpp"
#include "flecs.h"

namespace ps {
	inline static constexpr const char* UI_ROO_ID = "uiRoot";

	struct RootNode {};
	struct Node {};

	//struct Anchor : public sf::Vector2<float> {};
	//struct Pivot : public sf::Vector2<float> {};

	struct Style {
		sf::Vector2f size { 0.f, 0.f };
		sf::Color backgroundColor{ sf::Color::Transparent };
		sf::RectangleShape shape;
	};

	struct Interaction {
		enum class Type {
			None,
			Hovered,
			Clicked
		};

		Type type;
	};

	//using Text = sf::Text;
	struct Text {
		sf::Text* text;
	};

	struct Image {
		sf::Texture texture;
		sf::Image img;
		sf::Color color;
		std::string path;
	};

	using UiTraversalId = int;

	struct UiModule {
		UiModule(flecs::world& world);
	};
}
