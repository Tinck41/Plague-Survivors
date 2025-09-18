#pragma once

#include "flecs.h"

#include <string>
#include <memory>

namespace ps {
	class Font;

	struct Text {
		std::string string;
		std::shared_ptr<Font> font;
	};

	struct TextModule {
		TextModule(flecs::world& world);
	};
}
