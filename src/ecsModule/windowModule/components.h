#pragma once

#include "SDL3/SDL.h"
#include "ext/vector_int2.hpp"

namespace se {
	struct Window {
		enum class WindowState : std::uint8_t {
			Hidden,
			Present,
		};

		int width;
		int height;

		bool has_focus = true;

		SDL_Window* handle;
		SDL_GPUTexture* swapchain_texture;

		std::uint32_t window_id;

		WindowState window_state = WindowState::Present;

		static Window create(const char* title, int width, int height, SDL_WindowFlags flags = 0);
		static Window create(const char* title, glm::ivec2 size, SDL_WindowFlags flags = 0);
	};

	struct MainWindow {};
}
