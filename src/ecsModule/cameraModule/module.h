#pragma once

#include "flecs.h"
#include "vec2.hpp"

namespace ps {
	enum class CameraPhases {
		BeginCameraMode,
		EndCameraMode,
	};

	struct Camera {
		flecs::entity_t target;
		glm::vec2 center;
		glm::vec2 offset;
		float rotation;
		float zoom = 1.f;
	};

	struct CameraModule {
		CameraModule(flecs::world& world);
	};
}
