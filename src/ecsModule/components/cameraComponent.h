#pragma once

#include "SFML/System/Vector2.hpp"
#include "flecs.h"

namespace ps::ecsModule {
	struct CameraComponent {
		flecs::entity target = flecs::entity::null();
		sf::Vector2f center { 0.f, 0.f };
		sf::Vector2f offset { 0.f, 0.f };
	};

	struct CameraTransitionComponent {
		enum eTransitionType {
			SMOOTH,
			INSTANT,
		};
		flecs::entity newTarget;
		eTransitionType transitionType;
	};

	struct CameraShakingComponent {
		float duration = 1.f;
		sf::Vector2f horizontalOffset { 0.f, 0.f };
		sf::Vector2f verticalOffset { 0.f, 0.f };
	};
}
