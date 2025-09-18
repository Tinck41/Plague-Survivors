#pragma once

#include "flecs.h"
#include "glm.hpp"

#include <string>
#include <memory>

namespace ps {
	class Font;

	struct Text {
		std::string string;
		std::shared_ptr<Font> font;
		glm::vec4 color{ 255.f, 255.f, 255.f, 255.f };
	};

	struct TextModule {
		TextModule(flecs::world& world);
	};
}
