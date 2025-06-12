#pragma once

#include "SDL3/SDL.h"
#include "SDL3/SDL_render.h"
#include "utils/instanceBase.h"

#include <unordered_map>
#include <string>
#include <expected>

namespace ps {
	enum class TextureLoadError {
		TextureNotFound,
	};

	struct TextureAllocator {
		void operator() (SDL_Texture* texture) {
			SDL_DestroyTexture(texture);
		}
	};

	struct TextureDeleter {
		void operator() (SDL_Texture* texture) {
			SDL_DestroyTexture(texture);
		}
	};

	using Texture = SDL_Texture;
	using TextureRef = std::unique_ptr<SDL_Texture, TextureDeleter>;

	class ResourceManager : public utils::InstanceBase<ResourceManager> {
	public:
		TextureRef getTexture(const std::string& path);

		std::expected<Texture, TextureLoadError> loadTexture(const std::string& path);
	private:
		std::unordered_map<std::string, TextureRef> m_loadedTextures;
	};

	// TODO: unload texture
}
