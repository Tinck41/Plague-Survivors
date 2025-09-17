#pragma once

#include "flecs.h"
#include "SDL3/SDL.h"
#include "glm.hpp"

namespace ps {
	struct Application {
		SDL_Window* window;

		SDL_AppResult status = SDL_APP_CONTINUE;

		int target_fpg = 60;

		bool is_vsync_enabled = false;
		bool is_running = true;
		bool window_resized = false;
	};

	struct WindowResized {};

	struct AppModule {
		AppModule(flecs::world& world);
	};
}
