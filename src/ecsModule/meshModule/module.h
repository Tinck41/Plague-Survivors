#pragma once

#include "flecs.h"
#include "SDL3/SDL.h"
#include "glm.hpp"
#include "texture.h"

#include <memory>
#include <vector>

namespace ps {
	struct Vertex {
		glm::vec3 position;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f};
		glm::vec2 uv;
	};

	struct Mesh {
		std::vector<SDL_GPUBuffer> vertex_buffer;
		std::vector<SDL_GPUBuffer> index_buffer;
	};

	struct QuadMesh {
		SDL_GPUBuffer* vertex_buffer;
		SDL_GPUBuffer* index_buffer;
	};

	struct Material {
		SDL_GPUGraphicsPipeline* pipeline;
		std::shared_ptr<Texture> texture;
	};

	struct MeshModule {
		MeshModule(flecs::world& world);
	};
}
