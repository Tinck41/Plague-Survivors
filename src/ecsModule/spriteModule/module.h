#pragma once

#include "ecsModule/transformModule/module.h"
#include "flecs.h"
#include "SDL3/SDL.h"
#include "vec2.hpp"
#include "mat4x4.hpp"
#include "vec4.hpp"
#include "texture.h"

#include <optional>
#include <memory>
#include <map>
#include <variant>
#include <vector>

namespace ps {
	struct Sprite {
		glm::vec2 origin;
		std::optional<glm::vec2> custom_size;
		glm::vec4 color{ 1.f, 1.f, 1.f, 1.f };
		std::shared_ptr<Texture> texture;
	};

	struct SpriteSingle {
		std::optional<glm::vec2> custom_size;
	};

	struct SpriteSequence {
		std::pair<size_t, size_t> range;
	};

	using SpriteKind = std::variant<SpriteSingle, SpriteSequence>;

	struct SpriteRenderData {
		flecs::entity_t entity;
		std::shared_ptr<Texture> texture;
		Transform transform;
		glm::vec4 color;
		SpriteKind kind;
	};

	struct SpriteRangeRenderData {
		glm::vec3 position;
		glm::vec4 colour;
		glm::vec2 uv;
	};

	using SpritesRenderData = std::vector<SpriteRenderData>;
	using SpritesRangeRenderData = std::vector<SpriteRangeRenderData>;

	struct SpriteInstance {
		glm::vec4 translation;
		glm::vec4 rotation;
		glm::vec4 scale;
		glm::vec4 color;
		glm::vec2 uv;
		glm::vec2 size;
	};

	struct SpritePipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;

		std::shared_ptr<Texture> white_texture;
	};

	struct SpriteStorage {
		SDL_GPUBuffer* index_buffer;
		SDL_GPUBuffer* instance_buffer;

		SDL_GPUTransferBuffer* transfer_buffer;
	};

	struct SpriteBatch {
		uint32_t size;
		uint32_t first_instance;
		std::shared_ptr<Texture> texture;
	};

	using SpriteBatches = std::map<flecs::entity_t, SpriteBatch>;

	struct SpriteModule {
		SpriteModule(flecs::world& world);
	};
}
