#include "module.h"

#include "ecsModule/common.h"

using namespace ps;

void resetScene(flecs::world& ecs) {
	ecs.delete_with(flecs::ChildOf, ecs.entity<SceneRoot>());
}

SceneModule::SceneModule(flecs::world& world) {
	world.module<SceneModule>();

	world.component<ActiveScene>().add(flecs::Exclusive);
	world.component<SceneRoot>();
	world.component<MenuScene>();
	world.component<GameScene>();

	world.set<MenuScene>({
		.pip = world.pipeline()
			.with(flecs::System)
			.without<GameScene>()
			.build()
	});
	world.set<GameScene>({
		.pip = world.pipeline()
			.with(flecs::System)
			.without<MenuScene>()
			.build()
	});

	world.observer<ActiveScene>()
		.event(flecs::OnAdd)
		.second<MenuScene>()
		.each([](flecs::iter& it, size_t, ActiveScene) {
			flecs::world ecs = it.world();
			flecs::entity scene = ecs.component<SceneRoot>();

			resetScene(ecs);

			// TODO: load menu

			ecs.set_pipeline(ecs.get<MenuScene>()->pip);
		});

	world.observer<ActiveScene>()
		.event(flecs::OnAdd)
		.second<GameScene>()
		.each([](flecs::iter& it, size_t, ActiveScene) {
			auto ecs = it.world();
			auto scene = ecs.component<SceneRoot>();

			resetScene(ecs);

			// TODO: load game

			ecs.set_pipeline(ecs.get<GameScene>()->pip);
		});

	world.add<ActiveScene, MenuScene>();
}
