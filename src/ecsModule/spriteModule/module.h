#pragma once

#include "flecs.h"
#include "SDL3/SDL.h"
#include "vec2.hpp"

#include <optional>
#include <string>

namespace ps {
	struct SpritePipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUBuffer* vertex_buffer;
		SDL_GPUBuffer* index_buffer;
		SDL_GPUSampler* sampler;
	};

	struct Sprite {
		glm::vec2 texture_size;
		glm::vec2 origin;
		std::optional<glm::vec2> custom_size;
		SDL_FColor color;
		std::string path;
	};

	struct SpriteModule {
		SpriteModule(flecs::world& world);
	};
}
