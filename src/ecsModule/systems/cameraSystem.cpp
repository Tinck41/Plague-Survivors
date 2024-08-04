#include "cameraSystem.h"

#include "ecsModule/components/boxColliderComponent.h"
#include "ecsModule/components/cameraComponent.h"
#include "ecsModule/components/inputComponent.h"
#include "ecsModule/components/rendererComponent.h"
#include "ecsModule/components/timeComponent.h"
#include "ecsModule/components/transformComponent.h"
#include "ecsModule/components/velocityComponent.h"
#include "ecsModule/ecsController.h"
#include "ecsModule/types.h"

using namespace ps;
using namespace ecsModule;

void CameraSystem::create() {
	auto& world = EcsControllerInstance::getInstance()->getWorld();

	world.system<CameraComponent, CameraShakingComponent>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t size, CameraComponent& c, CameraShakingComponent& s) {
			if (s.duration > 0.f) {
				c.center.x += s.horizontalOffset.x;
				c.center.y += s.verticalOffset.y;
				s.duration -= it.delta_time();
				std::swap(s.horizontalOffset.x, s.horizontalOffset.y);
				std::swap(s.verticalOffset.x, s.verticalOffset.y);
			} else {
				it.entity(size).remove<CameraShakingComponent>();
			}
		});

	world.system<CameraComponent, CameraTransitionComponent>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t size, CameraComponent& c, CameraTransitionComponent& t) {
			c.target = t.newTarget;
			if (t.transitionType == CameraTransitionComponent::eTransitionType::INSTANT) {
				auto targetTransform = c.target.get<TransformComponent>();
				auto targetCollider = c.target.get<BoxColliderComponent>();

				c.center.x = targetTransform->translation.x + targetCollider->size.x * 0.5f;
				c.center.y = targetTransform->translation.y + targetCollider->size.y * 0.5f;
			}
			it.entity(size).remove<CameraTransitionComponent>();
		});

	world.system<CameraComponent, RendererComponent>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t size, CameraComponent& c, RendererComponent& r) {
			const auto targetTransform = c.target.get<TransformComponent>();
			const auto targetCollider = c.target.get<BoxColliderComponent>();
			const auto velocity = it.world().singleton<CameraComponent>().get<VelocityComponent>()->velocity;

			c.center.x += (targetTransform->translation.x + targetCollider->size.x * 0.5f - c.center.x) * velocity.x * it.delta_time();
			c.center.y += (targetTransform->translation.y + targetCollider->size.y * 0.5f - c.center.y) * velocity.y * it.delta_time();

			auto view = r.renderer->getView();
			view.setCenter(c.center);
			r.renderer->setView(view);
		});

	// Example
	world.system<InputComponent>()
		.term_at(0).singleton()
		.kind(flecs::OnUpdate)
		.each([](flecs::iter& it, size_t, InputComponent& i) {
			const auto& world = it.world();
			auto camera = world.singleton<CameraComponent>();
			if (i.keys[Key::C].released) {
				CameraTransitionComponent t;
				const auto target = camera.get<CameraComponent>()->target;
				const auto player = world.lookup("player");
				const auto obstacle = world.lookup("obstacle");
				t.newTarget = target == player ? obstacle : player;
				t.transitionType = CameraTransitionComponent::eTransitionType::SMOOTH;
				camera.set(t);
			} else if (i.keys[Key::V].released) {
				CameraTransitionComponent t;
				const auto target = camera.get<CameraComponent>()->target;
				const auto player = world.lookup("player");
				const auto obstacle = world.lookup("obstacle");
				t.newTarget = target == player ? obstacle : player;
				t.transitionType = CameraTransitionComponent::eTransitionType::INSTANT;
				camera.set(t);
			}
		});
}
