#pragma once

#include "ecsModule/mesh3d_module/components.h"
#include "ecsModule/render_module/render_phase.h"
#include "ecsModule/render_module/material.h"

#include "flecs.h"
#include "glm.hpp"

namespace se {
	struct ExtractedMesh3d {
		flecs::entity_t entity;
		flecs::entity_t material_id;
		SDL_GPUTexture* texture;
		Vertex3d* vertices;
		size_t vertices_num;
		std::uint32_t* indices;
		size_t indices_num;
		glm::mat4 transform;
		Color color;
	};

	struct Mesh3dStorageData {
		glm::mat4 model;
	};

	using ExtractedMeshes3d = std::vector<ExtractedMesh3d>;

	struct Mesh3dModule {
		Mesh3dModule(flecs::world& world);
	};

	se::Material create_mesh3d_material(flecs::world& world);
	se::PhaseContext create_mesh3d_context(flecs::world& world);
	se::RenderPhaseExtractor create_mesh3d_extractor(flecs::world& world, flecs::entity_t helper);
	se::RenderPhaseUploader create_mesh3d_uploader();

	glm::mat4 extract_3d_view(const Camera &camera, const GlobalTransform &transfrom);

	std::pair<uint64_t, uint64_t> request_mesh3d_allocate(Mesh3dAllocator& allocator, std::vector<Vertex3d> vertices, std::vector<uint32_t> indices);
}
