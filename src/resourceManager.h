#pragma once

#include "raylib.h"
#include "utils/instanceBase.h"

#include <unordered_map>
#include <string>

namespace ps {
	using TextureOpt = Texture*;
	using TextureHandle = std::shared_ptr<Texture>;

	class ResourceManager : public utils::InstanceBase<ResourceManager> {
	public:
		TextureHandle getTexture(std::string path);

		TextureOpt loadTexture(std::string path);
	private:
		std::unordered_map<std::string, TextureHandle> m_loadedTextures;
	};
}
