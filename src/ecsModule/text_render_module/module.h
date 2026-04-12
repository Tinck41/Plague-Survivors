#pragma once

#include "SDL3_ttf/SDL_ttf.h"
#include "color.h"
#include "flecs.h"
#include "ecsModule/render_module/render_phase.h"
#include "ecsModule/render_module/material.h"

#include <unordered_map>
#include <vector>

namespace ps {
	struct ExtractedText2d {
		flecs::entity_t entity;
		flecs::entity_t material_id;
		SDL_GPUTexture* texture;
		glm::vec2 translation;
		glm::vec2 scale;
		float font_scale;
		Color color;
		float outline_width;
		Color outline_color;
		TTF_GPUAtlasDrawSequence* seq;
	};

	using ExtractedText2ds = std::vector<ExtractedText2d>;

	struct TextRenderModule {
		TextRenderModule(flecs::world& world);
		~TextRenderModule();

		TTF_TextEngine* engine;
	};

	Material create_text_material(flecs::world& world);
	PhaseContext create_text_context(flecs::world& world);
	RenderPhaseExtractor create_text_extractor(flecs::world& world, flecs::entity_t helper);
	RenderPhaseUploader create_text_uploader();
}
