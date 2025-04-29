#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/sceneModule/module.h"

using namespace ps;

constexpr unsigned WINDOW_WIDTH  = 600;
constexpr unsigned WINDOW_HEIGHT = 600;

constexpr const char* WINDOW_TITLE = "Plague: Survivors";

AppModule::AppModule(flecs::world& world) {
	world.module<AppModule>();

	world.component<Application>();

	world.set<Application>({
		.window{sf::VideoMode{ sf::Vector2u{ WINDOW_WIDTH, WINDOW_HEIGHT } }, WINDOW_TITLE, sf::Style::Default, sf::State::Windowed}
	});

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

	world.entity(Phases::Display)
		.add(flecs::Phase)
		.depends_on(Phases::Render);
}
