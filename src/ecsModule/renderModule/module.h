#pragma once

#include "SDL3/SDL.h"
#include "flecs.h"
#include "texture.h"

#include <functional>
#include <vector>
#include <memory>

namespace se {
	struct RenderStats {
		int draw_calls = 0;
	};

	struct RenderDevice {
		SDL_GPUDevice* gpu;
	};
	
	struct CopyCommands {
		SDL_GPUCommandBuffer* buffer;
	};

	struct RenderCommands {
		SDL_GPUCommandBuffer* cmd_buffer;
		SDL_GPURenderPass* render_pass;
	};

	struct RenderPass {
		SDL_GPURenderPass* render_pass;
	};

	struct WhiteTexture {
		std::shared_ptr<Texture> texture;
	};

	struct RenderPhase {
		flecs::entity_t entity;

		std::function<void(flecs::entity_t, flecs::entity_t, const flecs::world&)> draw_function;

		float sort_value;

		size_t batch_size;
	};

	using RenderPhaseItems = std::unordered_map<flecs::entity_t, std::vector<RenderPhase>>;
	using CameraRenderPhaseItems = std::unordered_map<flecs::entity_t, RenderPhaseItems>;

	struct CameraCompositionPipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;
	};

	struct BindData {
		std::function<void(const flecs::world&)> function;
	};

	struct BindTexture {
		std::function<void(flecs::entity_t, const flecs::world&)> function;
	};

	struct RenderModule {
		RenderModule(flecs::world& world);

		inline static std::vector<flecs::entity_t> render_phases_order;
	};
}
