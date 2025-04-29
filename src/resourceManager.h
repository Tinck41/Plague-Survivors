#pragma once

#include "SFML/Graphics/Texture.hpp"
#include "utils/instanceBase.h"

namespace ps {
	using Texture = sf::Texture;
	using TextureOpt = Texture*;
	using TextureHandle = std::shared_ptr<Texture>;

	class ResourceManager : public utils::InstanceBase<ResourceManager> {
	public:
		TextureHandle getTexture(const std::string& path);

		TextureOpt loadTexture(const std::string& path);
	private:
		std::unordered_map<std::string, TextureHandle> m_loadedTextures;
	};
}
