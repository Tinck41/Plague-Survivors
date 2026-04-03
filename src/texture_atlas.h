#pragma once

#include "glm.hpp"
#include "SDL3/SDL.h"

#include <filesystem>
#include <unordered_map>
#include <vector>
#include <expected>

namespace ps {
	struct TextureAtlas {
		std::unordered_map<std::string, SDL_FRect> textures;

		static std::expected<TextureAtlas, std::string> from_json(const std::filesystem::path& path);
	};
}
