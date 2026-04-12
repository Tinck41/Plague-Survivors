#pragma once

#include "ecsModule/render_module/material.h"
#include "ecsModule/render_module/render_phase.h"

#include "flecs.h"

namespace ps {
	struct UiVertex {
		glm::vec3 position;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f};
		glm::vec2 uv;
		
		std::uint32_t flags;
	};

	enum class ShaderFlags : std::uint8_t {
		None = 0,
		Textured = 1 << 0,
	};

	Material create_node_material(flecs::world& world);
	PhaseContext create_node_context(flecs::world& world);
	RenderPhaseExtractor create_image_node_extractor(flecs::world& world, flecs::entity_t helper);
	RenderPhaseExtractor create_color_node_extractor(flecs::world& world, flecs::entity_t helper);
	RenderPhaseUploader create_node_uploader();
}
