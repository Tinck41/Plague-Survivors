#include "module.h"

#include "SFML/Graphics/RectangleShape.hpp"
#include "box2d/box2d.h"
#include "ecsModule/common.h"
#include "ecsModule/appModule/module.h"
#include "ecsModule/transformModule/module.h"

using namespace ps;

b2World physics{ b2Vec2{ 0.f, 0.f } };

constexpr int32 VELOCITY_INTERACTIONS = 6;
constexpr int32 POSITION_INTERACTIONS = 2;
constexpr float PPM = 100.f;

PhysicsModule::PhysicsModule(flecs::world& world) {
	world.module<PhysicsModule>();

	//TODO
	//world.component<BoxCollider>().add(flecs::With, world.component<Rigidbody>());
	world.component<Rigidbody>();
	world.component<BoxCollider>();

	world.system()
		.kind(flecs::OnUpdate)
		.each([](flecs::iter& it, size_t) {
			physics.Step(it.delta_time(), VELOCITY_INTERACTIONS, POSITION_INTERACTIONS);
		});

	world.observer<BoxCollider, Rigidbody, Transform*>()
		.with<Rigidbody>().filter()
		.event(flecs::OnSet)
		.each([](BoxCollider& b, Rigidbody& r, Transform* t) {
			b2BodyDef bodyDef;
			bodyDef.type          = r.Type == Rigidbody::BodyType::Dynamic ? b2BodyType::b2_dynamicBody : b2BodyType::b2_staticBody;
			bodyDef.position      = t ? b2Vec2{ (t->translation.x + b.size.x / 2) / PPM, (t->translation.y + b.size.y / 2) / PPM } : b2Vec2{ b.size.x / 2 / PPM, b.size.y / 2 / PPM };
			//bodyDef.angle         = t ? t->rotation : 0;
			bodyDef.fixedRotation = r.fixedRotation;

			auto body = physics.CreateBody(&bodyDef);

			b2PolygonShape polygon;
			polygon.SetAsBox((b.size.x * (t ? t->scale.x : 1.f)) / 2.f / PPM, (b.size.y * (t ? t->scale.y : 1.f)) / 2.f / PPM);

			b2FixtureDef fixtureDef;
			fixtureDef.shape       = &polygon;
			fixtureDef.density     = b.dencity;
			fixtureDef.friction    = b.friction;
			fixtureDef.restitution = b.restitution;
			//fixtureDef.restitutionThreshold = b.restitutionThreshold;

			b.fixture = body->CreateFixture(&fixtureDef);

			r.body = body;
		});

	world.system<Velocity, Rigidbody>()
		.kind(Phases::Update)
		.each([](Velocity& v, Rigidbody& r) {
			auto body = (b2Body*)r.body;
			body->SetLinearVelocity({ v.x / PPM, v.y / PPM });
		});

	world.system<Transform, const Rigidbody, const BoxCollider>()
		.kind(Phases::Update)
		.each([](flecs::entity e, Transform& t, const Rigidbody& r, const BoxCollider& b) {
			const auto body = (b2Body*)r.body;
			const auto& position = body->GetPosition();

			t.translation.x = position.x * PPM - b.size.x / 2;
			t.translation.y = position.y * PPM - b.size.y / 2;
			//t.rotation = body->GetAngle();
		});
}
