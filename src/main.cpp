#include "ecsModule/renderModule/module.h"
#include "ecsModule/tweenModule/module.h"
#include "ecsModule/uiModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/playerModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/appModule/module.h"
#include "ecsModule/sceneModule/module.h"
#include "resourceManager.h"

#include "flecs.h"

int main() {
	ps::ResourceManager::init(std::make_unique<ps::ResourceManager>());

	flecs::world world;

	world.import<ps::AppModule>();
	world.import<ps::RenderModule>();
	world.import<ps::SceneModule>();
	world.import<ps::TimeModule>();
	world.import<ps::TransformModule>();
	world.import<ps::InputModule>();
	world.import<ps::UiModule>();
	world.import<ps::PhysicsModule>();
	world.import<ps::CameraModule>();
	world.import<ps::PlayerModule>();
	world.import<ps::TweenModule>();
	world.import<flecs::stats>();

	world.set<flecs::Rest>({});

	world.script().filename("assets/scripts/app.flecs").run();
	//world.script().filename("assets/scripts/mainMenu.flecs").run();
	world.script().filename("assets/scripts/gameScene.flecs").run();

	while(!WindowShouldClose()) {
		world.progress(GetFrameTime());
	}
}
