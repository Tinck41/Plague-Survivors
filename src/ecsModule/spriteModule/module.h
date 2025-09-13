#pragma once

#include "flecs.h"
#include "SDL3/SDL.h"
#include "vec2.hpp"
#include "mat4x4.hpp"
#include "vec4.hpp"
#include "texture.h"

#include <optional>
#include <string>
#include <memory>

namespace ps {
	struct Sprite {
		glm::vec2 texture_size;
		glm::vec2 origin;
		std::optional<glm::vec2> custom_size;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f };
		std::string path;
		std::shared_ptr<Texture> texture;
	};

	struct SpriteData {
		alignas(16) glm::vec3 position;
		float rotation;
		glm::vec2 scale;
		glm::vec2 padding;
		float u, v, w, h;
		glm::vec4 color;
	};
	
	struct SpritePipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;
	};

	struct SpriteRenderer {
		std::shared_ptr<Texture> texture;
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;
		SDL_GPUBuffer* buffer;
		SDL_GPUTransferBuffer* transfer_buffer;
	};

	struct SpriteModule {
		SpriteModule(flecs::world& world);
	};
}
