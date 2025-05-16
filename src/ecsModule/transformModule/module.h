#pragma once

#include "flecs.h"
#include "vec3.hpp"
#include "mat4x4.hpp"

namespace ps {
	struct Dirty {};

	struct Transform {
		glm::vec3 translation{ 0.f, 0.f, 0.f };
		glm::vec3 scale{ 1.f, 1.f, 1.f };
		glm::vec3 rotation;

		glm::mat4 matrix;
	};

	struct GlobalTransform : public Transform {};

	struct TransformModule {
		TransformModule(flecs::world& world);
	};
}
