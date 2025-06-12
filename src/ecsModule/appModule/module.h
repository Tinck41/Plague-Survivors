#pragma once

#include "flecs.h"
#include "SDL3/SDL.h"

namespace ps {
	struct Application {
		SDL_Window* window;
		SDL_Renderer* renderer;

		SDL_AppResult status = SDL_APP_CONTINUE;
		int targetFPS = 60;
		bool isVSyncEnabled = false;
	};

	struct AppModule {
		AppModule(flecs::world& world);
	};
}
