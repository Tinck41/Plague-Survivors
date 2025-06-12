#pragma once

#include "SDL3/SDL.h"
#include "mat4x4.hpp"
#include "flecs.h"
#include "vec2.hpp"
#include "vec3.hpp"
#include "vec4.hpp"

#include <optional>
#include <vector>
#include <string>

namespace ps {
	namespace internal {
		struct VertexData {
			glm::vec3 position;
			glm::vec4 color;
		};

		struct UBO {
			alignas(16) glm::mat4 mvp;
		};
	}

	struct Renderer {
		SDL_GPUDevice* gpu;
		SDL_GPUGraphicsPipeline* pipeline;
		glm::mat4 projection;
	};

	struct SpritePipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUBuffer* vertex_buffer;
		SDL_GPUBuffer* index_buffer;
	};

	struct Drawable {
		std::vector<SDL_Vertex> verts;
		std::vector<int> indices;
		std::weak_ptr<SDL_Texture> texture;
	};

	struct Sprite {
		std::shared_ptr<SDL_Texture> texture;
		SDL_FColor color{ 1.f, 1.f, 1.f, 1.f };
		std::optional<glm::vec2> custom_size;
		std::string path;
		glm::vec2 origin;
	};

	struct Circle {
		float radius;
		SDL_FColor color;
	};

	struct Rectangle {
		glm::vec2 size;
		SDL_FColor color;
	};

	struct RenderModule {
		RenderModule(flecs::world& world);
	};
}
