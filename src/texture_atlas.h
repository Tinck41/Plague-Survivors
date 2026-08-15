#pragma once

#include "SDL3/SDL.h"

#include <vector>

namespace se {
	struct TextureAtlas {
		std::vector<SDL_FRect> rects;
		size_t current_index;
	};
}
