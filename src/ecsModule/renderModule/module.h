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

	struct RenderDevice {
		SDL_GPUDevice* gpu;
	};

	struct RenderPass {
		SDL_GPUCommandBuffer* cmd_buffer;
		SDL_GPURenderPass* render_pass;
		SDL_GPUTexture* swapchain_texture;
	};

	struct WhiteTexture {
		SDL_GPUTexture* texture;
	};

	struct Renderer {
		SDL_GPUDevice* gpu;
		SDL_GPUTexture* white_texture;
	};

	struct ModelMatrix {
		glm::mat4 matrix;
	};

	struct RenderModule {
		RenderModule(flecs::world& world);
	};
}
