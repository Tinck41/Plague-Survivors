#pragma once

#include "SDL3/SDL.h"

#include <string>

namespace ps {
	void print_sdl_error();

	SDL_GPUShader* load_shader(SDL_GPUDevice& gpu, std::string_view path, uint32_t num_uniform_buf = 0);
}
