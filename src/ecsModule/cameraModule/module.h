#pragma once

#include "SFML/System/Vector2.hpp"
#include "flecs.h"

namespace ps {
	struct Camera {
		flecs::entity target;
		sf::Vector2f center;
		sf::Vector2f offset;
	};

	struct CameraTransition {
		enum eTransitionType {
			SMOOTH,
			INSTANT,
		};
		flecs::entity newTarget;
		eTransitionType transitionType;
	};

	struct CameraShaking {
		float duration = 1.f;
		sf::Vector2f horizontalOffset { 0.f, 0.f };
		sf::Vector2f verticalOffset { 0.f, 0.f };
	};

	struct CameraModule {
		CameraModule(flecs::world& world);
	};
}
