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
		glm::vec2 offset;
	};

	struct CameraModule {
		CameraModule(flecs::world& world);
	};
}
