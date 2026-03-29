#pragma once

#include "SDL3/SDL.h"

namespace ps {
	struct Window {
		int width;
		int height;

		bool has_focus = true;

		SDL_Window* handle;
		SDL_GPUTexture* swapchain_texture;
	};

	struct MainWindow {};
}
