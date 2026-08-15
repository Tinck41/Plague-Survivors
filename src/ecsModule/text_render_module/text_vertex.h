#pragma once

#include "glm.hpp"

namespace se {
	struct TextVertex {
		glm::vec3 position;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f};
		glm::vec2 uv;
		float outline_width;
		glm::vec4 outline_color;
	};
}
