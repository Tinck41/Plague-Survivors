#pragma once

#include "glm.hpp"

namespace ps {
	struct DefaultUniform {
		glm::mat4 view_proj;

		glm::vec3 resolution;

		float time;
		float delta_time;
	};
}
