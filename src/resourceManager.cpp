#include "resourceManager.h"

using namespace ps;

TextureHandle ResourceManager::getTexture(std::string path) {
	if (m_loadedTextures.contains(path)) {
		return m_loadedTextures.at(path);
	}

	if (auto texture = loadTexture(path)) {
		m_loadedTextures.emplace(path, std::move(texture));

		return m_loadedTextures.at(path);
	}

	return nullptr;
}

TextureOpt ResourceManager::loadTexture(std::string path) {
	auto texture = new Texture(LoadTexture(path.c_str()));

	return texture;
}
