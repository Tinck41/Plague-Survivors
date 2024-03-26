#include "physicsSystem.h"

#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_world.h"
#include "ecsModule/components/transformComponent.h"
#include "ecsModule/ecsController.h"
#include "ecsModule/components/rigidbodyComponent.h"
#include "ecsModule/components/boxColliderComponent.h"
#include "ecsModule/components/physicsComponent.h"
#include "ecsModule/types.h"

using namespace ps;
using namespace ecsModule;

void PhysicsSystem::create() {
	auto& world = EcsControllerInstance::getInstance()->getWorld();

	world.system<PhysicsComponent>()
		.term_at(1).singleton()
		.kind(flecs::OnUpdate)
		.each([](flecs::iter& it, size_t, PhysicsComponent& p) {
			p.world->Step(it.delta_time(), p.velocityInterations, p.positionInterations);
		});

	world.observer<PhysicsComponent, RigidbodyComponent, BoxColliderComponent, TransformComponent>()
		.term_at(1).singleton()
		.term_at(2).filter()
		.event(flecs::OnSet)
		.each([](PhysicsComponent& p, RigidbodyComponent& r, BoxColliderComponent& b, TransformComponent& t) {
			b2BodyDef bodyDef;
			bodyDef.type = r.Type == RigidbodyComponent::BodyType::Dynamic ? b2BodyType::b2_dynamicBody : b2BodyType::b2_staticBody;
			bodyDef.position = { (t.trasnslation.x / 2.f) / 30.f , (t.trasnslation.y  / 2.f) / 30.f };
			bodyDef.angle = t.rotation;
			bodyDef.fixedRotation = r.fixedRotation;

			auto body = p.world->CreateBody(&bodyDef);

			b2PolygonShape polygon;
			polygon.SetAsBox((b.size.x * t.scale.x) / 4.f / 30.f, (b.size.y * t.scale.y) / 4.f / 30.f);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &polygon;
			fixtureDef.density = b.dencity;
			fixtureDef.friction = b.friction;
			fixtureDef.restitution = b.restitution;
			//fixtureDef.restitutionThreshold = b.restitutionThreshold;

			body->CreateFixture(&fixtureDef);

			r.body = body;
		});

	world.system<TransformComponent, RigidbodyComponent>()
		.kind(Phases::Update)
		.each([](TransformComponent& t, const RigidbodyComponent& r) {
			auto body = (b2Body*)r.body;
			const auto& position = body->GetPosition();
			t.trasnslation.x = position.x * 2.f * 30.f;
			t.trasnslation.y = position.y * 2.f * 30.f;
			t.rotation = body->GetAngle();
		});

	world.set<PhysicsComponent>({});
}
