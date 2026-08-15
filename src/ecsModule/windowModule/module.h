#pragma once

#include "flecs.h"
#include "SDL3/SDL.h"

namespace se {
	struct WindowModule {
		WindowModule(flecs::world& world);

		SDL_Window* main_window;
	};
}
