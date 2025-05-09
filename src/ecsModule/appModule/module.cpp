#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/sceneModule/module.h"
#include "raylib.h"

using namespace ps;

constexpr unsigned WINDOW_WIDTH  = 600;
constexpr unsigned WINDOW_HEIGHT = 600;

constexpr const char* WINDOW_TITLE = "Plague: Survivors";

AppModule::AppModule(flecs::world& world) {
	world.module<AppModule>();

	world.component<Application>();

	world.add<Application>();

	world.entity(Phases::OnStart)
		.add(flecs::Phase)
		.depends_on(flecs::OnStart);

	world.entity(Phases::HandleInput)
		.add(flecs::Phase)
		.depends_on(flecs::PostLoad);

	world.entity(Phases::PreUpdate)
		.add(flecs::Phase)
		.depends_on(flecs::PreUpdate);

	world.entity(Phases::Update)
		.add(flecs::Phase)
		.depends_on(flecs::OnUpdate);

	world.entity(Phases::PostUpdate)
		.add(flecs::Phase)
		.depends_on(flecs::PostUpdate);

	world.entity(Phases::Clear)
		.add(flecs::Phase)
		.depends_on(Phases::PostUpdate);

	world.entity(Phases::Render)
		.add(flecs::Phase)
		.depends_on(Phases::Clear);

	world.entity(Phases::RenderUI)
		.add(flecs::Phase)
		.depends_on(Phases::Clear);

	world.entity(Phases::Display)
		.add(flecs::Phase)
		.depends_on(Phases::RenderUI);

	world.observer<Application>()
		.event(flecs::OnAdd)
		.each([](Application& app) {
			if (app.isVSyncEnabled) {
				SetConfigFlags(FLAG_VSYNC_HINT);
			}
			SetConfigFlags(FLAG_WINDOW_RESIZABLE);
			SetTargetFPS(app.targetFPS);
		});
}
