#include "module.h"

#include "SDL3_image/SDL_image.h"
#include "ecsModule/appModule/module.h"
#include "ecsModule/common.h"

#include <unordered_map>

using namespace ps;

std::unordered_map<std::string, std::shared_ptr<Texture>> textures;

void AssetStorage::set_renderer(SDL_Renderer* renderer) {
	m_renderer = renderer;
}

std::shared_ptr<Texture> AssetStorage::load_texture(const std::string& path) const {
	if (!textures.contains(path)) {
		textures[path] = std::shared_ptr<SDL_Texture>(IMG_LoadTexture(m_renderer, path.c_str()), SDL_DestroyTexture);
	}

	return textures.at(path);
}

AssetModule::AssetModule(flecs::world& world) {
	world.module<AssetModule>();

	world.import<AppModule>();

	world.component<AssetStorage>();

	world.system<Application, AssetStorage>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(Phases::OnStart)
		.each([](Application& app, AssetStorage& storage) {
			storage.set_renderer(app.renderer);
		});

	world.system<AssetStorage>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](AssetStorage& storage) {
			auto it = textures.begin();

			while (it != textures.end()) {
				if (it->second.use_count() == 0) {
					it = textures.erase(it);
				} else {
					++it;
				}
			}
		});

	world.add<AssetStorage>();
}

