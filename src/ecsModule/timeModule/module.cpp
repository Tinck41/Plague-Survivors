#include "module.h"

#include "raylib.h"

using namespace ps;

TimeModule::TimeModule(flecs::world& world) {
	world.module<TimeModule>();

	world.component<Time>()
		.add(flecs::Singleton)
		.member<float>("deltaTime");

	world.observer<Time>()
		.event(flecs::OnSet)
		.event(flecs::OnAdd)
		.each([](Time& t) {
			t.deltaTime = GetFrameTime();
		});

	world.system<Time>()
		.kind(flecs::OnStore)
		.each([](Time& t) {
			t.deltaTime = GetFrameTime();
		});

	world.set<Time>({});
}
