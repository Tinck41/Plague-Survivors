#pragma once

#include "ecsModule/renderModule/module.h"
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

	struct CollectedSpriteItem {
		flecs::entity_t entity;
		std::shared_ptr<Texture> texture;
		Transform transform;
		glm::vec4 color;
		glm::vec2 size;
		SpriteKind kind;
	};

	struct CollectedSpriteItems {
		std::vector<CollectedSpriteItem> items;
		std::unordered_map<flecs::entity_t, size_t> lookup;
	};

	using CameraCollectedSpriteItems = std::map<flecs::entity_t, CollectedSpriteItems>;

	struct SpriteRangeRenderData {
		glm::vec3 position;
		glm::vec4 colour;
		glm::vec2 uv;
	};

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
	using CameraSpriteBatches = std::map<flecs::entity_t, SpriteBatches>;

	struct Transparent2d : public RenderPhase {};

	struct SpriteModule {
		SpriteModule(flecs::world& world);
	};
}
