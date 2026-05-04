#pragma once

#include "glm.hpp"

namespace ps {
	struct DefaultUniform {
		glm::mat4 view_proj;

		glm::ivec2 viewport;

		float time;
		float delta_time;
	};
}
