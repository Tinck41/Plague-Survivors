#pragma once

#include "flecs.h"

#include "texture.h"
#include "font.h"

#include <filesystem>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ps {
	class AssetStorage {
	public:
		AssetStorage() = default;

		void update();

		std::shared_ptr<Texture> load_texture(SDL_GPUDevice& gpu, const std::string& path);
		std::shared_ptr<Font> load_font(const std::string& path);
		std::shared_ptr<Font> load_font(const std::string& path, float size);
	private:
		std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
		std::unordered_map<std::string, std::shared_ptr<Font>> fonts;
	};

	struct AssetModule {
		AssetModule(flecs::world& world);
	};
}
