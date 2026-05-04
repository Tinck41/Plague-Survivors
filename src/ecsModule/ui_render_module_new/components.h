#pragma once

#include "SDL3_ttf/SDL_ttf.h"
#include "color.h"
#include "flecs.h"
#include "texture.h"

#include <vector>
#include <memory>

namespace ps {
	struct ExtractedTextNode {
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

	using ExtractedTextNodes = std::vector<ExtractedTextNode>;

	struct ExtractedNode {
		flecs::entity_t entity;
		flecs::entity_t material_id;
		SDL_GPUTexture* texture;
		glm::vec2 texture_size;
		glm::vec2 translation;
		glm::vec2 rotation;
		glm::vec2 scale;
		Color color;
		glm::vec2 size;
		glm::vec2 uv;
		glm::vec4 border_color;
		float border_radius;
		float border_width;
	};

	using ExtractedNodes = std::vector<ExtractedNode>;
}
