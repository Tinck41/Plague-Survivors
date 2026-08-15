#pragma once

#include "flecs.h"
#include "glm.hpp"

#include "ecsModule/render_module/render_phase.h"
#include "ecsModule/render_module/material.h"

#include <memory>
#include <unordered_map>

namespace se {
	struct ExtractedSprite {
		flecs::entity_t entity;
		flecs::entity_t material_id;
		SDL_GPUTexture* texture;
		glm::vec2 texture_size;
		glm::vec2 translation;
		glm::vec2 rotation;
		glm::vec2 scale;
		glm::vec4 color;
		glm::vec2 size;
		glm::vec2 uv;
	};

	struct CirlceUniform {
		float radius;
		float softness;
		glm::vec2 center;
	};

	using ExtractedSprites = std::vector<ExtractedSprite>;

	struct SpriteRenderModule {
		SpriteRenderModule(flecs::world& world);
	};

	Material create_sprite_material(flecs::world& world);
	PhaseContext create_sprite_context(flecs::world& world);
	RenderPhaseExtractor create_sprite_extractor(flecs::world& world, flecs::entity_t helper);
	RenderPhaseUploader create_sprite_uploader();
}
