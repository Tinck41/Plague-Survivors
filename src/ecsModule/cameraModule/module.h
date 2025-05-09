#pragma once

#include "raylib.h"
#include "flecs.h"
#include "vec2.hpp"

namespace ps {
	enum class CameraPhases {
		BeginCameraMode,
		EndCameraMode,
	};

	struct Camera {
		flecs::entity target;
		glm::vec2 center;
		glm::vec2 offset;
		float rotation;
		float zoom = 1.f;
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
		glm::vec2 horizontalOffset { 0.f, 0.f };
		glm::vec2 verticalOffset { 0.f, 0.f };
	};

	struct CameraModule {
		CameraModule(flecs::world& world);
	};
}
