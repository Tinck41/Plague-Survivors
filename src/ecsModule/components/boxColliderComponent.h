#pragma once

#include "SFML/System/Vector2.hpp"

namespace ps::ecsModule {
	struct BoxColliderComponent {
		sf::Vector2f offset = { 0.f, 0.f };
		sf::Vector2f size = { 0.f, 0.f };

		float dencity = 1.f;
		float friction = 0.f;
		float restitution = 0.f;
		float restitutionThreshold = 0.f;

		void* fixture = nullptr;
	};
}
