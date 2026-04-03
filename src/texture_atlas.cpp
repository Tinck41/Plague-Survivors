#include "texture_atlas.h"

#include "glaze/glaze.hpp"

using namespace ps;

struct TexturePartDescriptor {
	std::string name;
	float x;
	float y;
	float width;
	float height;
};

struct TextureAtlasInterenal {
	std::vector<TexturePartDescriptor> atlas;
};

std::expected<TextureAtlas, std::string> TextureAtlas::from_json(const std::filesystem::path& path) {
	auto atlas_file = static_cast<char*>(SDL_LoadFile(path.c_str(), nullptr));
	auto result = glz::read_json<TextureAtlasInterenal>(atlas_file);

	SDL_free(atlas_file);

	if (!result) {
		return std::unexpected(glz::format_error(result, atlas_file));
	}

	TextureAtlas atlas;

	for (const auto& part : result.value().atlas) {
		atlas.textures[part.name] = SDL_FRect{ part.x, part.y, part.width, part.height };
	}

	return atlas;
}
