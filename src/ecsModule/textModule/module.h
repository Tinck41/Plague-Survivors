#pragma once

#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "flecs.h"
#include "glm.hpp"

#include <string>
#include <memory>
#include <unordered_map>

namespace ps {
	class Font;

	struct Text {
		std::string string;
		std::shared_ptr<Font> font;
		glm::vec4 color{ 255.f, 255.f, 255.f, 255.f };
	};

	struct TextStorage {
		SDL_GPUBuffer* vertex_buffer;
		SDL_GPUBuffer* index_buffer;

		SDL_GPUTransferBuffer* transfer_buffer;

		std::unordered_map<std::string, TTF_Text*> text_data;
	};

	struct TextPipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;

		TTF_TextEngine* engine;
	};

	using TextBatches = std::unordered_map<flecs::entity_t, TTF_GPUAtlasDrawSequence*>;

	struct TextModule {
		TextModule(flecs::world& world);
	};
}
