#include "module.h"

#include "SDL3_ttf/SDL_ttf.h"

using namespace ps;

TextModule::TextModule(flecs::world& world) {
	world.module<TextModule>();

	world.component<Text>();
}
