#include "module.h"

#include "raylib.h"

using namespace ps;

TimeModule::TimeModule(flecs::world& world) {
	world.module<TimeModule>();

	world.component<Time>();

	world.set<Time>({});

	world.system<Time>()
		.term_at(0).singleton()
		.kind(flecs::OnStore)
		.each([](Time& t) {
			t.deltaTime = GetFrameTime();
		});
}
