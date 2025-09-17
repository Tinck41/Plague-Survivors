#include "module.h"

#include "ecsModule/common.h"
#include "spdlog/spdlog.h"
#include "ecsModule/cameraModule/module.h"

using namespace ps;

constexpr unsigned WINDOW_WIDTH  = 600;
constexpr unsigned WINDOW_HEIGHT = 600;

constexpr const char* WINDOW_TITLE = "Plague: Survivors";

AppModule::AppModule(flecs::world& world) {
	world.module<AppModule>();

	world.component<Application>().add(flecs::Singleton);

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

	//world.observer<Application>()
	//	.event(flecs::OnAdd)
	//	.each([](Application& app) {
	//		if (app.isVSyncEnabled) {
	//			SetConfigFlags(FLAG_VSYNC_HINT);
	//		}
	//		SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	//		SetTargetFPS(app.targetFPS);
	//	});

	world.system<Application>()
		.kind(Phases::OnStart)
		.each([](flecs::iter& it, size_t i, Application& app) {
			SDL_SetAppMetadata("Plague: Survivors", "1.0.0", "");

			if (!SDL_Init(SDL_INIT_VIDEO)) {
				app.status = SDL_APP_FAILURE;

				return;
			}

			app.window = SDL_CreateWindow("Plague: Survivors", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

			if (!app.window) {
				app.status = SDL_APP_FAILURE;

				return;
			}

			SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
			SDL_SetLogOutputFunction(
				[](void *userdata, int category, SDL_LogPriority priority, const char *message) {
					switch(priority) {
						case SDL_LOG_PRIORITY_INVALID: { spdlog::info("{}", message); break; };
						case SDL_LOG_PRIORITY_TRACE: { spdlog::info("{}", message); break; };
						case SDL_LOG_PRIORITY_VERBOSE: { spdlog::info("{}", message); break; };
						case SDL_LOG_PRIORITY_DEBUG: { spdlog::debug("{}", message); break; };
						case SDL_LOG_PRIORITY_INFO: { spdlog::info("{}", message); break; };
						case SDL_LOG_PRIORITY_WARN: { spdlog::warn("{}", message); break; };
						case SDL_LOG_PRIORITY_ERROR: { spdlog::error("{}", message); break; };
						case SDL_LOG_PRIORITY_CRITICAL: { spdlog::critical("{}", message); break; };
						case SDL_LOG_PRIORITY_COUNT: { spdlog::info("{}", message); break; };
					}
				},
				nullptr
			);

			app.status = SDL_APP_CONTINUE;
		});

	world.add<Application>();
}
