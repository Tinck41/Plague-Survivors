#pragma once

#include "flecs.h"
#include "SDL3/SDL.h"

#include <string>
#include <memory>

namespace ps {
	using Texture = SDL_Texture;

	class AssetStorage {
	public:
		AssetStorage() = default;

		void set_renderer(SDL_Renderer* renderer);

		std::shared_ptr<Texture> load_texture(const std::string& path) const;
	private:
		SDL_Renderer* m_renderer = nullptr;
	};

	struct AssetModule {
		AssetModule(flecs::world& world);
	};
}
