#pragma once

#include "SDL3/SDL.h"
#include "mat4x4.hpp"
#include "flecs.h"
#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"

#include "texture.h"

#include <functional>
#include <optional>
#include <vector>
#include <string>
#include <memory>

namespace ps {
	struct RenderDevice {
		SDL_GPUDevice* gpu;
	};
	
	struct CopyCommands {
		SDL_GPUCommandBuffer* buffer;
	};

	struct RenderCommands {
		SDL_GPUCommandBuffer* cmd_buffer;
		SDL_GPURenderPass* render_pass;
		SDL_GPUTexture* swapchain_texture;
	};

	struct WhiteTexture {
		SDL_GPUTexture* texture;
	};

	struct RenderItem {
		flecs::entity_t entity;
		float sort_value;
		std::function<void(flecs::entity_t, const flecs::world&)> draw_function;
	};

	using RenderItems = std::vector<RenderItem>;

	struct RenderModule {
		RenderModule(flecs::world& world);
	};
}
