#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/appModule/module.h"

using namespace ps;

#define SPRING_CONSTANT 0.3f

float CriticallyDampedSpring( float a_Target,
                              float a_Current,
                              float & a_Velocity,
                              float a_TimeStep )
{
    float currentToTarget = a_Target - a_Current;
    float springForce = currentToTarget * SPRING_CONSTANT;
    float dampingForce = -a_Velocity * 2 * sqrt( SPRING_CONSTANT );
    float force = springForce + dampingForce;
    a_Velocity += force * a_TimeStep;
    float displacement = a_Velocity * a_TimeStep;
    return a_Current + displacement;
}

float lerp(float v0, float v1, float t) {
	return v0 * (1 - t) + v1 * t;
}

CameraModule::CameraModule(flecs::world& world) {
	world.module<CameraModule>();

	world.component<Camera>();
	world.component<CameraShaking>();
	world.component<CameraTransition>();

	world.set<Camera>({});
	world.singleton<Camera>().set(Velocity{ 5.f, 5.f });

	world.system<Camera, CameraShaking>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t size, Camera& c, CameraShaking& s) {
			if (s.duration > 0.f) {
				c.center.x += s.horizontalOffset.x;
				c.center.y += s.verticalOffset.y;
				s.duration -= it.delta_time();
				std::swap(s.horizontalOffset.x, s.horizontalOffset.y);
				std::swap(s.verticalOffset.x, s.verticalOffset.y);
			} else {
				it.entity(size).remove<CameraShaking>();
			}
		});

	world.system<Camera, CameraTransition>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t size, Camera& c, CameraTransition& t) {
			c.target = t.newTarget;
			if (t.transitionType == CameraTransition::eTransitionType::INSTANT) {
				const auto targetTransform = c.target.get<GlobalTransform>();
				const auto targetCollider = c.target.get<BoxCollider>();

				c.center.x = targetTransform->translation.x + targetCollider->size.x * 0.5f;
				c.center.y = targetTransform->translation.y + targetCollider->size.y * 0.5f;
			}
			it.entity(size).remove<CameraTransition>();
		});

	world.system<Camera, Application>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t size, Camera& c, Application& app) {
			if (c.target == flecs::entity::null()) {
				return;
			}
			const auto targetPosition = c.target.get<GlobalTransform >()->translation;
			const auto velocity = it.world().singleton<Camera>().get<Velocity>();

			//c.center.x += (targetTransform->translation.x + /*targetCollider->size.x * 0.5f*/ - c.center.x) * velocity->x * it.delta_time();
			//c.center.y += (targetTransform->translation.y + /*targetCollider->size.y * 0.5f*/ - c.center.y) * velocity->y * it.delta_time();
			//c.center.x = targetTransform->translation.x; // + /*targetCollider->size.x * 0.5f*/ - c.center.x;
			//c.center.y = targetTransform->translation.y; // + /*targetCollider->size.y * 0.5f*/ - c.center.y;
			//c.center.x = c.center.x * (1 - it.delta_time() * velocity->x) + targetTransform->translation.x * it.delta_time() * velocity->x;
			//c.center.y = c.center.y * (1 - it.delta_time() * velocity->y) + targetTransform->translation.y * it.delta_time() * velocity->y;

			//c.center.x = std::floor(lerp(c.center.x, targetTransform->translation.x, velocity->x * it.delta_time()));
			//c.center.y = std::floor(lerp(c.center.y, targetTransform->translation.y, velocity->y * it.delta_time()));

			c.center.x = targetPosition.x + c.offset.x;
			c.center.y = targetPosition.y + c.offset.y;

			auto view = app.window.getView();
			view.setCenter(c.center);
			app.window.setView(view);
		});

	// Example
	world.system<Camera, Input>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(flecs::OnUpdate)
		.each([world](flecs::iter& it, size_t size, Camera& camera, Input& input) {
			if (input.keys[Key::C].released) {
				CameraTransition t;
				const auto target = camera.target;
				const auto player = world.lookup("ps::PlayerModule::player");
				const auto obstacle = world.lookup("ps::PlayerModule::obstacle");
				t.newTarget = target == player ? obstacle : player;
				t.transitionType = CameraTransition::eTransitionType::SMOOTH;
				world.singleton<Camera>().set(t);
			} else if (input.keys[Key::V].released) {
				CameraTransition t;
				const auto target = camera.target;
				const auto player = world.lookup("ps::PlayerModule::player");
				const auto obstacle = world.lookup("ps::PlayerModule::obstacle");
				t.newTarget = target == player ? obstacle : player;
				t.transitionType = CameraTransition::eTransitionType::INSTANT;
				world.singleton<Camera>().set(t);
			}
		});
}
