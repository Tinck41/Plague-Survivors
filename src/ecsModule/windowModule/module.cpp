#include "module.h"

#include "components.h"
#include "ecsModule/common.h"

using namespace ps;

constexpr unsigned WINDOW_WIDTH  = 600;
constexpr unsigned WINDOW_HEIGHT = 600;

WindowModule::WindowModule(flecs::world& world) {
	world.module<WindowModule>();

	world.component<Window>()
		.add(flecs::Singleton)
		.add(flecs::Exclusive);

	world.system<Window>()
		.kind(Phases::OnStart)
		.each([](Window& window) {
			window.handle = SDL_CreateWindow("Plague: Survivors", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
		});

	world.add<Window>();
}
