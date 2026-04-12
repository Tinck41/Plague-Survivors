#pragma once

#include "SDL3/SDL.h"
#include "flecs.h"

#include <vector>

namespace ps {
	struct Material {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;

		std::vector<flecs::entity_t> vertex_uniforms;
		std::vector<flecs::entity_t> fragment_uniforms;
	};
}
