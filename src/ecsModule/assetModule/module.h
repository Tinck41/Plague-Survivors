#pragma once

#include "flecs.h"
#include "SDL3/SDL.h"
#include "vec2.hpp"

#include "texture.h"

#include <string>
#include <memory>
#include <unordered_map>

namespace ps {
	class AssetStorage {
	public:
		AssetStorage() = default;

		void update();

		std::shared_ptr<Texture> load_texture(SDL_GPUDevice& gpu, const std::string& path);
	private:
		std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
	};

	struct AssetModule {
		AssetModule(flecs::world& world);
	};
}
