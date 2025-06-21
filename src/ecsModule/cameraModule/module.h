#pragma once

#include "flecs.h"
#include "vec2.hpp"
#include "mat4x4.hpp"

namespace ps {
	enum class CameraPhases {
		BeginCameraMode,
		EndCameraMode,
	};

	struct Camera {
		glm::vec2 offset;
		glm::mat4 projection;
	};

	struct CameraModule {
		CameraModule(flecs::world& world);

		inline static flecs::entity_t EcsCamera;
	};
}
