#include "ecsModule/renderModule/module.h"
#include "ecsModule/uiModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/spriteModule/module.h"
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
	//world.import<ps::SceneModule>();
	world.import<ps::TimeModule>();
	world.import<ps::TransformModule>();
	world.import<ps::InputModule>();
	//world.import<ps::UiModule>();
	world.import<ps::PhysicsModule>();
	world.import<ps::CameraModule>();
	world.import<ps::SpriteModule>();
	//world.import<ps::PlayerModule>();

	world.set<flecs::Rest>({});

	world.app().enable_rest().run();

	//const auto *app = world.get<ps::Application>();
	//while(app->window.isOpen()) {
	//	world.progress(world.get<ps::Time>()->deltaTime);
	//}
}
