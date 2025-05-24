#include "module.h"

#include "raylib.h"

using namespace ps;

TimeModule::TimeModule(flecs::world& world) {
	world.module<TimeModule>();

	world.component<Time>()
		.member<float>("deltaTime");

	world.observer<Time>()
		.event(flecs::OnSet)
		.event(flecs::OnAdd)
		.each([](Time& t) {
			t.deltaTime = GetFrameTime();
		});

	world.system<Time>()
		.term_at(0).singleton()
		.kind(flecs::OnStore)
		.each([](Time& t) {
			t.deltaTime = GetFrameTime();
		});

	world.set<Time>({});
}
