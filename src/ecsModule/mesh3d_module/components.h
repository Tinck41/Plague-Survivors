#pragma once

#include "texture.h"
#include "tiny_obj_loader.h"
#include "arena.h"

#include <memory>

namespace se {
	struct Vertex3d {
		glm::vec3 position;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f};
		glm::vec2 uv;
	};

	struct Mesh3d {
		std::shared_ptr<Texture> texture;
		Color color;

		std::vector<Vertex3d> vertices;
		std::vector<std::uint32_t> indices;

		std::uint64_t vertex_index;
		std::uint64_t vertex_count;

		uint64_t request_id = UINT64_MAX;
		uint64_t vertex_pos = 0;
	};

	struct Mesh3dUniform {
		glm::mat4 model;
	};

	struct Mesh3dDescription {
		enum class OptimizationFlags : uint8_t {
			None               = 0,
			Indexing           = 1 << 0,
			VertexCache        = 1 << 1,
			Overdraw           = 1 << 2,
			VertexFetch        = 1 << 3,
			VertexQuantization = 1 << 4,
			IndexFiltering     = 1 << 5,
			ShadowIndexing     = 1 << 6,

			All = Indexing | VertexCache | Overdraw | VertexFetch | VertexQuantization | IndexFiltering | ShadowIndexing,
		};

		friend constexpr OptimizationFlags operator|(OptimizationFlags lhs, OptimizationFlags rhs) {
			using base_t = std::underlying_type_t<OptimizationFlags>;

			return static_cast<OptimizationFlags>(
				static_cast<base_t>(lhs) | static_cast<base_t>(rhs)
			);
		}
		friend constexpr OptimizationFlags operator&(OptimizationFlags lhs, OptimizationFlags rhs) {
			using base_t = std::underlying_type_t<OptimizationFlags>;

			return static_cast<OptimizationFlags>(
				static_cast<base_t>(lhs) & static_cast<base_t>(rhs)
			);
		}

		enum class Format : std::uint8_t {
			obj,
			fbx,
			gltf,
		};

		std::string texture_path;
		std::string model_path;

		Format model_format;

		OptimizationFlags optimization_flags = OptimizationFlags::All;
	};

	struct Mesh3dAllocator {
		struct AllocateRequest {
			std::vector<Vertex3d> vertices;
			std::vector<uint32_t> indices;
		};

		struct ReuploadRequest {
			std::vector<Vertex3d> vertices;
			uint64_t pos;
		};

		std::vector<AllocateRequest> allocate_requests;
		std::vector<ReuploadRequest> reupload_requests;

		uint64_t last_allocate_pos = 0;
	};
}
