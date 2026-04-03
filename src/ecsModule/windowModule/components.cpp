#include "components.h"

using namespace ps;

Window Window::create(const char *title, int width, int height, SDL_WindowFlags flags) {
	auto window = SDL_CreateWindow(title, width, height, flags);
	return {
		.width = width,
		.height = height,
		.handle = window,
		.window_id = SDL_GetWindowID(window),
	};
}

Window Window::create(const char *title, glm::ivec2 size, SDL_WindowFlags flags) {
	return create(title, size.x, size.y, flags);
}

