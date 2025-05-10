#include "module.h"

#include "box2d/box2d.h"
#include "ecsModule/common.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/transformModule/module.h"

using namespace ps;

constexpr float PPM = 100.f;

PhysicsModule::PhysicsModule(flecs::world& world) {
	world.module<PhysicsModule>();

	world.import<TransformModule>();
	world.import<TimeModule>();

	world.component<Physics>();
	world.component<Velocity>();
	world.component<Rigidbody>();
	world.component<BoxCollider>().add(flecs::With, world.component<Rigidbody>());

	world.component<b2WorldId>()
		.member<uint16_t>("index1")
		.member<uint16_t>("generation");

	world.component<b2BodyId>()
		.member<int32_t>("index1")
		.member<uint16_t>("world0")
		.member<uint16_t>("generation");

	world.component<Physics>()
		.member<b2WorldId>("worldId")
		.member<glm::vec2>("gravity")
		.member<int>("subStepCount");

	world.component<Rigidbody::BodyType>()
		.constant("Static", Rigidbody::BodyType::Static)
		.constant("Dynamic", Rigidbody::BodyType::Dynamic)
		.constant("Kinematic", Rigidbody::BodyType::Kinematic);

	world.component<Rigidbody>()
		.member<Rigidbody::BodyType>("type")
		.member<bool>("fixedRotation")
		.member<b2BodyId>("bodyId");

	world.component<BoxCollider>()
		.member<glm::vec2>("offset")
		.member<glm::vec2>("size")
		.member<float>("dencity")
		.member<float>("friction")
		.member<float>("restitution")
		.member<float>("restitutionThreshold");

	world.component<Velocity>()
		.member<float>("x")
		.member<float>("y");

	world.observer<Physics>()
		.event(flecs::OnAdd)
		.each([](Physics& p) {
			b2WorldDef worldDef = b2DefaultWorldDef();

			worldDef.gravity = { p.gravity.x, p.gravity.y };

			p.worldId = b2CreateWorld(&worldDef);
		});

	world.observer<Physics>()
		.event(flecs::OnRemove)
		.each([](Physics& p) {
			b2DestroyWorld(p.worldId);

			p.worldId = b2_nullWorldId;
		});

	world.system<Physics, Time>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(flecs::OnUpdate)
		.each([](Physics& p, Time& t) {
			b2World_Step(p.worldId, t.deltaTime, p.subStepCount);
		});

	world.observer<Physics, BoxCollider, Rigidbody, Transform*>()
		.term_at(0).singleton()
		//.with<Rigidbody>().filter()
		.event(flecs::OnSet)
		.each([](Physics& p, BoxCollider& b, Rigidbody& r, Transform* t) {
			auto bodyDef = b2DefaultBodyDef();

			bodyDef.type          = r.type == Rigidbody::BodyType::Dynamic ? b2BodyType::b2_dynamicBody : b2BodyType::b2_staticBody;
			bodyDef.position      = t ? b2Vec2{ (t->translation.x + b.size.x / 2) / PPM, (t->translation.y + b.size.y / 2) / PPM } : b2Vec2{ b.size.x / 2 / PPM, b.size.y / 2 / PPM };
			//bodyDef.angle         = t ? t->rotation : 0;
			bodyDef.fixedRotation = r.fixedRotation;

			r.bodyId = b2CreateBody(p.worldId, &bodyDef);

			auto polygon = b2MakeBox((b.size.x * (t ? t->scale.x : 1.f)) / 2.f / PPM, (b.size.y * (t ? t->scale.y : 1.f)) / 2.f / PPM);

			auto shapeDef = b2DefaultShapeDef();

			shapeDef.density              = b.dencity;
			shapeDef.material.friction    = b.friction;
			shapeDef.material.restitution = b.restitution;
			//fixtureDef.restitutionThreshold = b.restitutionThreshold;

			b2CreatePolygonShape(r.bodyId, &shapeDef, &polygon);
		});

	world.system<Velocity, Rigidbody>()
		.kind(Phases::Update)
		.each([](Velocity& v, Rigidbody& r) {
			b2Body_SetLinearVelocity(r.bodyId, { v.x / PPM, v.y / PPM });
		});

	world.system<Transform, const Rigidbody, const BoxCollider>()
		.kind(Phases::Update)
		.each([](flecs::entity e, Transform& t, const Rigidbody& r, const BoxCollider& b) {
			const auto& position = b2Body_GetPosition(r.bodyId);

			t.translation.x = position.x * PPM - b.size.x / 2;
			t.translation.y = position.y * PPM - b.size.y / 2;
			//t.rotation = body->GetAngle();
		});

	world.add<Physics>();
}
