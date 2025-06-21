#include "ecsModule/appModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/cameraModule/module.h"

#include "flecs.h"
#include "spdlog/spdlog.h"

//static void flecs_log_callback(int32_t level, const char *file, int32_t line, const char *msg) {
//
//}

int main() {
	flecs::world world;
	//ecs_os_get_api().log_ = flecs_log_callback;

	world.import<ps::AppModule>();
	world.import<ps::AssetModule>();
	world.import<ps::TransformModule>();
	world.import<ps::CameraModule>();
	world.import<ps::RenderModule>();
	world.import<ps::SpriteModule>();
	//world.import<ps::SceneModule>();
	//world.import<ps::TimeModule>();
	//world.import<ps::InputModule>();
	//world.import<ps::UiModule>();
	//world.import<ps::PhysicsModule>();
	//world.import<ps::PlayerModule>();
	//world.import<ps::TweenModule>();
	world.import<flecs::stats>();

	world.set<flecs::Rest>({});

	//world.script().filename("assets/scripts/app.flecs").run();
	//world.script().filename("assets/scripts/mainMenu.flecs").run();
	//world.script().filename("assets/scripts/gameScene.flecs").run();

	while(world.progress()) {
		//spdlog::info("----------------------tick---------------------");
	}
}
