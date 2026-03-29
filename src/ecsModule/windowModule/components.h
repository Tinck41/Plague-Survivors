#pragma once

#include "SDL3/SDL.h"
#include "ext/vector_int2.hpp"

namespace ps {
	struct Window {
		int width;
		int height;

		bool has_focus = true;

		SDL_Window* handle;
		SDL_GPUTexture* swapchain_texture;

		static Window create(const char* title, int width, int height, SDL_WindowFlags flags = 0);
		static Window create(const char* title, glm::ivec2 size, SDL_WindowFlags flags = 0);
	};

	struct MainWindow {};
}
