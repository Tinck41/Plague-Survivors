#pragma once

#include "glm.hpp"

namespace ps {
	struct Mesh;

	struct Rectangle {
		glm::vec2 size;

		operator Mesh();
	};

	struct Circle {
		float radius;

		operator Mesh();
	};

}
