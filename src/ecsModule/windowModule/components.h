#pragma once

#include "SDL3/SDL_video.h"

namespace ps {
	struct Window {
		SDL_Window* handle;

		int width;
		int height;
	};
}
