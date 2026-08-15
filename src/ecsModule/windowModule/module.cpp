#include "module.h"

#include "components.h"
#include "ecsModule/common.h"

using namespace se;

constexpr unsigned WINDOW_WIDTH  = 600;
constexpr unsigned WINDOW_HEIGHT = 600;

WindowModule::WindowModule(flecs::world& world) {
	world.module<WindowModule>();

	world.component<Window>()
		.member<int>("width")
		.member<int>("height")
		.member<bool>("has_focus");

	world.component<MainWindow>();

	main_window = SDL_CreateWindow("Plague: Survivors", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

	world.entity("main_window")
		.add<MainWindow>()
		.set<Window>({
			.width = WINDOW_WIDTH,
			.height = WINDOW_HEIGHT,
			.handle = main_window,
		});
}
