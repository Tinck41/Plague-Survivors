#include "module.h"

#include "components.h"
#include "ecsModule/common.h"

using namespace ps;

constexpr unsigned WINDOW_WIDTH  = 600;
constexpr unsigned WINDOW_HEIGHT = 600;

WindowModule::WindowModule(flecs::world& world) {
	world.module<WindowModule>();

	world.component<Window>()
		.member<int>("width")
		.member<int>("height")
		.add(flecs::Singleton);

	world.system<Window>()
		.kind(Phases::OnStart)
		.each([](Window& window) {
			window.handle = SDL_CreateWindow("Plague: Survivors", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

			window.width = WINDOW_WIDTH;
			window.height = WINDOW_HEIGHT;
		});

	world.observer<Window>()
		.event(flecs::OnSet)
		.each([](Window& window) {
			if (window.handle) {
				SDL_SetWindowSize(window.handle, window.width, window.height);
			}
		});

	world.add<Window>();
}
