#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/transformModule/module.h"

#include "raylib.h"
#include <iostream>

using namespace ps;

Camera2D g_camera{ 0 };

CameraModule::CameraModule(flecs::world& world) {
	world.module<CameraModule>();

	world.import<PhysicsModule>();
	world.import<TransformModule>();
	world.import<TimeModule>();
	world.import<RenderModule>();

	world.entity(CameraPhases::BeginCameraMode)
		.add(flecs::Phase)
		.depends_on(Phases::Clear);

	world.entity(CameraPhases::EndCameraMode)
		.add(flecs::Phase)
		.depends_on(Phases::Render);

	world.component<Camera>()
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<Velocity>())
		.add(flecs::Exclusive);

	world.component<Camera>()
		.member<glm::vec2>("offset");

	world.system<Window, Camera, Transform>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](Window& w, Camera& c, Transform& t) {
			g_camera.target = Vector2{ t.translation.x, t.translation.y };
			g_camera.zoom   = t.scale.x;
			g_camera.offset = Vector2{ c.offset.x + w.width * 0.5f, c.offset.y + w.height * 0.5f };
		});

	world.system<Camera>()
		.kind(CameraPhases::BeginCameraMode)
		.each([](Camera& c) {
			BeginMode2D(g_camera);
		});

	world.system<Camera>()
		.kind(CameraPhases::EndCameraMode)
		.each([](Camera& c) {
			EndMode2D();
		});

	EcsCamera = world.entity("EcsCamera");
}
