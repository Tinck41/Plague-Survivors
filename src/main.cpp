#include "ecsModule/appModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/meshModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/common.h"

#include "flecs.h"
#include "spdlog/spdlog.h"

int main() {
	flecs::world world;
	world.import<ps::AppModule>();
	world.import<ps::AssetModule>();
	world.import<ps::TransformModule>();
	world.import<ps::CameraModule>();
	world.import<ps::RenderModule>();
	world.import<ps::TextModule>();
	world.import<ps::SpriteModule>();
	world.import<ps::MeshModule>();
	world.import<ps::InputModule>();
	//world.import<ps::SceneModule>();
	//world.import<ps::TimeModule>();
	//world.import<ps::UiModule>();
	//world.import<ps::PhysicsModule>();
	//world.import<ps::PlayerModule>();
	//world.import<ps::TweenModule>();
	world.import<flecs::stats>();

	world.set<flecs::Rest>({});

	//world.script().filename("assets/scripts/app.flecs").run();
	//world.script().filename("assets/scripts/mainMenu.flecs").run();
	//world.script().filename("assets/scripts/gameScene.flecs").run();
	world.script().filename("assets/scripts/sandbox.flecs").run();

	world.system<ps::Application, ps::AssetStorage, ps::RenderDevice>()
		.kind(ps::Phases::OnStart)
		.each([world](ps::Application& app, ps::AssetStorage& storage, ps::RenderDevice& device) {
			world.entity().set<ps::Text>({ .string = "hello SDL3_ttf!", .font = storage.load_font("assets/FreeSans.ttf", 46) });
		});

	auto& app = world.get<ps::Application>();

	while(app.is_running) {
		//spdlog::info("----------------------tick---------------------");
		world.progress();
	}
}
