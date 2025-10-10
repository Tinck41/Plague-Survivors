#include "application.h"

#include "SDL3/SDL.h"
#include "app_state.h"
#include "ecsModule/common.h"
#include "spdlog/spdlog.h"

using namespace ps;

Application Application::create() {
	Application app;

	if (!app.init_sdl()) {
		throw std::runtime_error(SDL_GetError());
	}

	app.init_phases();

	app.world.component<AppState>()
		.add(flecs::Exclusive)
		.add(flecs::Singleton);

	app.world.set<flecs::Rest>({});
	app.world.set<AppState>(AppState::Continue);

	return app;
}

void Application::run() {
	world.progress();

	while (world.get<AppState>() == AppState::Continue) {
		world.progress();
	}
}

bool Application::init_sdl() {
	SDL_SetAppMetadata("Plague: Survivors", "1.0.0", "");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		return false;
	}

	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
	SDL_SetLogOutputFunction(
		[](void *userdata, int category, SDL_LogPriority priority, const char *message) {
			switch(priority) {
				case SDL_LOG_PRIORITY_INVALID:  { spdlog::info("{}", message);     break; };
				case SDL_LOG_PRIORITY_TRACE:    { spdlog::info("{}", message);     break; };
				case SDL_LOG_PRIORITY_VERBOSE:  { spdlog::info("{}", message);     break; };
				case SDL_LOG_PRIORITY_DEBUG:    { spdlog::debug("{}", message);    break; };
				case SDL_LOG_PRIORITY_INFO:     { spdlog::info("{}", message);     break; };
				case SDL_LOG_PRIORITY_WARN:     { spdlog::warn("{}", message);     break; };
				case SDL_LOG_PRIORITY_ERROR:    { spdlog::error("{}", message);    break; };
				case SDL_LOG_PRIORITY_CRITICAL: { spdlog::critical("{}", message); break; };
				case SDL_LOG_PRIORITY_COUNT:    { spdlog::info("{}", message);     break; };
			}
		},
		nullptr
	);

	return true;
}

void Application::init_phases() {
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

	world.entity(Phases::PreRender)
		.add(flecs::Phase)
		.depends_on(Phases::Clear);

	world.entity(Phases::Render)
		.add(flecs::Phase)
		.depends_on(Phases::PreRender);

	world.entity(Phases::PostRender)
		.add(flecs::Phase)
		.depends_on(Phases::Render);

	world.entity(Phases::RenderUI)
		.add(flecs::Phase)
		.depends_on(Phases::PostRender);

	world.entity(Phases::Display)
		.add(flecs::Phase)
		.depends_on(Phases::RenderUI);
}
