#pragma once 

#include "SFML/Window/Keyboard.hpp"
#include "ecsModule/components/inputComponent.h"

namespace ps::ecsModule {
	class InputSystem {
	public:
		static void create();
	private:
		InputSystem() = default;

		static Key SfmlKeyToPsKey(sf::Keyboard::Key key);
	};
}
