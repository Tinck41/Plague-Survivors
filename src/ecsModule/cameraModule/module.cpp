#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/transformModule/module.h"

#include "raylib.h"

using namespace ps;

float lerp(float v0, float v1, float t) {
	return v0 * (1 - t) + v1 * t;
}

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
		.add(flecs::Exclusive);

	world.component<Camera>()
		.member<flecs::entity_t>("target")
		.member<glm::vec2>("offset");

	world.system<Window, Time, Camera, Transform, Velocity>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t size, Window& w, Time& time, Camera& c, Transform& t, Velocity& v) {
			const auto position = c.target != flecs::entity::null() ? it.world().entity(c.target).get<GlobalTransform>()->translation : glm::vec3{ 0.f };

			t.translation.x = std::floor(lerp(t.translation.x, position.x, v.x * time.deltaTime));
			t.translation.y = std::floor(lerp(t.translation.y, position.y, v.y * time.deltaTime));

			g_camera.target   = Vector2{ t.translation.x, t.translation.y };
			//g_camera.rotation = t.rotation;
			g_camera.zoom     = t.scale.x;
			g_camera.offset   = Vector2{ c.offset.x + w.width * 0.5f, c.offset.y + w.height * 0.5f };
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

	world.set<Camera>({});
	world.singleton<Camera>().set(Velocity{ 7.f, 7.f });
}
