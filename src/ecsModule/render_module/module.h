#pragma once

#include "ecsModule/render_module/render_phase.h"
#include "flecs.h"
#include "SDL3/SDL.h"

#include <memory>

namespace ps {
	struct RenderStats {
		int draw_calls = 0;
	};

	struct RenderDevice {
		SDL_GPUDevice* gpu;
	};

	struct WhiteTexture {
		std::shared_ptr<Texture> texture;
	};

	struct RenderModule {
		RenderModule(flecs::world& world);
	};

	RenderPhaseSorter create_default_sorter();
	RenderPhaseRenderer create_default_renderer();
}
