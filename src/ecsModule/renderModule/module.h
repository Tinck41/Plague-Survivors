#pragma once

#include "SDL3/SDL.h"
#include "mat4x4.hpp"
#include "flecs.h"
#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"

#include "texture.h"

#include <optional>
#include <vector>
#include <string>
#include <memory>

namespace ps {
	namespace internal {
		struct VertexData {
			glm::vec3 position;
			glm::vec4 color;
			glm::vec2 uv;
		};

		struct UBO {
			alignas(16) glm::mat4 mvp;
		};
	}

	struct Renderer {
		SDL_GPUDevice* gpu;
		SDL_GPUTexture* white_texture;
	};

	struct Mesh {
		SDL_GPUBuffer* vertex_buffer;
		SDL_GPUBuffer* index_buffer;
	};

	struct Material {
		SDL_GPUGraphicsPipeline* pipeline;
		std::shared_ptr<Texture> texture;
		SDL_GPUSampler* sampler;
	};

	struct ModelMatrix {
		glm::mat4 matrix;
	};

	struct RenderModule {
		RenderModule(flecs::world& world);
	};
}
