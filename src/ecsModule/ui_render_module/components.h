#pragma once

#include "ecsModule/transformModule/module.h"
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "flecs.h"
#include "texture.h"
#include "glm.hpp"
#include "color.h"

#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ps {
	struct NodeSingle {
		std::optional<glm::vec2> custom_size;
	};

	struct NodeSequence {
		std::pair<size_t, size_t> range;
	};

	using NodeKind = std::variant<NodeSingle, NodeSequence>;

	struct CollectedUiItem {
		flecs::entity_t entity;
		std::shared_ptr<Texture> texture;
		Transform transform;
		Color color;
		glm::vec2 size;
		NodeKind kind;
	};

	struct Test {
		SDL_FRect rect;
		glm::vec2 size;
		glm::vec2 local_pos;
	};

	using TestSeq = std::vector<Test>;

	struct CollectedUiItems {
		std::vector<CollectedUiItem> items;
		std::unordered_map<flecs::entity_t, size_t> lookup;
	};

	using CameraCollectedUiItems = std::unordered_map<flecs::entity_t, CollectedUiItems>;

	struct UiPipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;
	};

	struct UiStorage {
		SDL_GPUBuffer* index_buffer;
		SDL_GPUBuffer* vertex_buffer;

		SDL_GPUTransferBuffer* transfer_buffer;
	};

	struct UiBatch {
		uint32_t size;
		uint32_t first_index;
		std::shared_ptr<Texture> texture;
	};

	struct UiTextStorage {
		SDL_GPUBuffer* vertex_buffer;
		SDL_GPUBuffer* index_buffer;

		SDL_GPUTransferBuffer* transfer_buffer;
	};

	struct UiTextPipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;

		TTF_TextEngine* engine;
	};

	struct CollectedUiTextItem {
		flecs::entity_t entity;
		std::shared_ptr<Texture> texture;
		Transform transform;
		Color color;
		TTF_Text* ttf_data;
		float scale;
	};

	struct CollectedUiTextItems {
		std::vector<CollectedUiTextItem> items;
		std::unordered_map<flecs::entity_t, size_t> lookup;
	};

	using CameraCollectedUiTextItems = std::unordered_map<flecs::entity_t, CollectedUiTextItems>;

	struct UiTextBatch {
		size_t index_offset = 0;
		size_t num_indices = 0;

		SDL_GPUTexture* texture; // TODO
	};

	using UiTextBatches = std::unordered_map<flecs::entity_t, std::vector<UiTextBatch>>;
	using CameraUiTextBatches = std::unordered_map<flecs::entity_t, UiTextBatches>;

	struct TransparentUi {};

	using UiBatches = std::unordered_map<flecs::entity_t, UiBatch>;
	using CameraUiBatches = std::unordered_map<flecs::entity_t, UiBatches>;
}
