#pragma once

#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "ecsModule/transformModule/module.h"
#include "flecs.h"
#include "texture.h"
#include "color.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ps {
	class Font;

	struct Text2d : public std::string {
		using std::string::string;

		Text2d(std::string string) : std::string(std::move(string)) {}
	};

	struct TextFont {
		std::shared_ptr<Font> handle;
	};

	struct TextColor : public Color {
		using Color::Color;

		TextColor(const Color& color) : Color(color) {}
	};

	struct TextData {
		glm::ivec2 size;

		TTF_Text* ttf_data;
	};

	struct TextStorage {
		SDL_GPUBuffer* vertex_buffer;
		SDL_GPUBuffer* index_buffer;

		SDL_GPUTransferBuffer* transfer_buffer;
	};

	struct TextPipeline {
		SDL_GPUGraphicsPipeline* pipeline;
		SDL_GPUSampler* sampler;

		TTF_TextEngine* engine;
	};

	struct CollectedTextItem {
		flecs::entity_t entity;
		std::shared_ptr<Texture> texture;
		Transform transform;
		Color color;
		TTF_Text* ttf_data;
	};

	struct CollectedTextItems {
		std::vector<CollectedTextItem> items;
		std::unordered_map<flecs::entity_t, size_t> lookup;
	};

	using CameraCollectedTextItems = std::unordered_map<flecs::entity_t, CollectedTextItems>;

	struct TextBatch {
		size_t index_offset = 0;
		size_t num_indices = 0;

		SDL_GPUTexture* texture; // TODO
	};

	using TextBatches = std::unordered_map<flecs::entity_t, std::vector<TextBatch>>;
	using CameraTextBatches = std::unordered_map<flecs::entity_t, TextBatches>;

	struct TextModule {
		TextModule(flecs::world& world);
	};
}
