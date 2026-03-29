#include "components.h"

using namespace ps;

Window Window::create(const char *title, int width, int height, SDL_WindowFlags flags) {
	return {
		.width = width,
		.height = height,
		.handle = SDL_CreateWindow(title, width, height, flags),
	};
}

Window Window::create(const char *title, glm::ivec2 size, SDL_WindowFlags flags) {
	return create(title, size.x, size.y, flags);
}

