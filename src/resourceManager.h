#pragma once

#include "raylib.h"
#include "utils/instanceBase.h"

#include <unordered_map>
#include <string>
#include <expected>

namespace ps {
	enum class TextureLoadError {
		TextureNotFound,
	};

	using TextureHandle = std::shared_ptr<Texture>;

	class ResourceManager : public utils::InstanceBase<ResourceManager> {
	public:
		TextureHandle getTexture(const std::string& path);

		std::expected<Texture, TextureLoadError> loadTexture(const std::string& path);
	private:
		std::unordered_map<std::string, TextureHandle> m_loadedTextures;
	};

	// TODO: unload texture
}
