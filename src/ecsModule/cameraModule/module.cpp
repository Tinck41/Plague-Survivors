#include "module.h"

#include "ecsModule/common.h"
//#include "ecsModule/physicsModule/module.h"
//#include "ecsModule/renderModule/module.h"
//#include "ecsModule/timeModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/appModule/module.h"
#include "ext/matrix_clip_space.hpp"

using namespace ps;

CameraModule::CameraModule(flecs::world& world) {
	world.module<CameraModule>();

	//world.import<PhysicsModule>();
	world.import<TransformModule>();
	world.import<AppModule>();
	//world.import<TimeModule>();
	//world.import<RenderModule>();

	world.component<Camera>()
		.add(flecs::With, world.component<Transform>())
//		.add(flecs::With, world.component<Velocity>())
		.add(flecs::Exclusive);

	world.component<Camera>()
		.member<glm::vec2>("offset");

	world.system<Application, Camera>()
		.term_at(0).singleton()
		.kind(Phases::OnStart)
		.each([](Application& app, Camera& c) {
			int width;
			int height;
			SDL_GetWindowSize(app.window, &width, &height);
			c.projection = glm::ortho(0.f, static_cast<float>(width), static_cast<float>(height), 0.f, -1.f, 1.f);
		});

	//world.system<Window, Camera, Transform>()
	//	.term_at(0).singleton()
	//	.kind(Phases::Update)
	//	.each([](Window& w, Camera& c, Transform& t) {
	//		g_camera.target = Vector2{ t.translation.x, t.translation.y };
	//		g_camera.zoom   = t.scale.x;
	//		g_camera.offset = Vector2{ c.offset.x + w.width * 0.5f, c.offset.y + w.height * 0.5f };
	//	});

	EcsCamera = world.entity("EcsCamera").add<Camera>();
}
