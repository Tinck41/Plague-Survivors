#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/editor_module/immediate.h"
#include "ecsModule/windowModule/module.h"
#include "spdlog/spdlog.h"

using namespace ps;

EditorModule::EditorModule(flecs::world& world) {
	world.module<EditorModule>();

	world.component<editor::Immediate>()
		.member("clear", &editor::Immediate::clear_on_frame_end)
		.add(flecs::Singleton);

	world.system<editor::Immediate>()
		.kind(Phases::OnStart)
		.each([&] (editor::Immediate& im) {
			im.init(world);
		});

	world.system<editor::Immediate>()
		.kind(Phases::PreUpdate)
		.each([&] (editor::Immediate& im) {
			im.begin_frame();
		});

	world.system<editor::Immediate>()
		.kind(Phases::Update)
		.immediate()
		.each([] (flecs::iter& it, size_t, editor::Immediate& im) {
			auto main_window = it.world().get<WindowModule>().main_window;
			glm::ivec2 size;
			SDL_GetWindowSize(main_window, &size.x, &size.y);
			it.world().defer_suspend();
			im.begin(
				"dockspace",
				editor::Immediate::Window::Flags::NoResize |
				editor::Immediate::Window::Flags::NoMove |
				editor::Immediate::Window::Flags::NoTitlebar |
				editor::Immediate::Window::Flags::NoClose |
				editor::Immediate::Window::Flags::NoCollapse, 
				{}, size
			);
			im.dockspace();
			im.end();
			im.begin("test");

			if (im.button("test_button")) {
				spdlog::info("pressed");
			}

			im.end();

			im.begin("test_dock");

			im.text("just to check");

			im.end();
			it.world().defer_resume();
		});

	world.system<editor::Immediate>()
		.kind(Phases::Update)
		.immediate()
		.each([] (flecs::iter& it, size_t, editor::Immediate& im) {
			it.world().defer_suspend();
			im.end_frame();
			it.world().defer_resume();
		});

	world.add<editor::Immediate>();
}
