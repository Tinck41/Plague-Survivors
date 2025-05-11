#pragma once

#include "flecs.h"
#include "vec2.hpp"

namespace ps {
	enum class CameraPhases {
		BeginCameraMode,
		EndCameraMode,
	};

	struct Camera {
		glm::vec2 offset;
	};

	struct CameraModule {
		CameraModule(flecs::world& world);

		inline static flecs::entity_t EcsCamera;
	};
}
