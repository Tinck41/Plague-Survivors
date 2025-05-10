#include "resourceManager.h"

#include "spdlog/spdlog.h"

using namespace ps;

TextureHandle ResourceManager::getTexture(const std::string& path) {
	if (m_loadedTextures.contains(path)) {
		return m_loadedTextures.at(path);
	}

	const auto texture = loadTexture(path);

	if (texture) {
		m_loadedTextures[path] = std::make_shared<Texture>(*texture);

		return m_loadedTextures.at(path);
	} else {
		spdlog::error("[ResourceManager::getTexture]: texture [{}] not found", path);
	}

	return nullptr;
}

std::expected<Texture, TextureLoadError> ResourceManager::loadTexture(const std::string& path) {
	const auto texture = LoadTexture(path.c_str());

	if (texture.id == 0) {
		return std::unexpected(TextureLoadError::TextureNotFound);
	}

	return texture;
}
