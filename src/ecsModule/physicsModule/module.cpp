#include "module.h"

#include "box2d/box2d.h"
#include "box2d/math_functions.h"
#include "ecsModule/common.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "trigonometric.hpp"

using namespace ps;

constexpr float PPM = 100.f;

PhysicsModule::PhysicsModule(flecs::world& world) {
	world.module<PhysicsModule>();

	world.import<TransformModule>();
	world.import<TimeModule>();

	world.component<Physics>();
	world.component<Velocity>();
	world.component<b2BodyId>();
	world.component<Rigidbody>()
		.add(flecs::With, world.component<b2BodyId>());
	world.component<BoxCollider>()
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<Rigidbody>());

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
		.member<bool>("fixedRotation");

	world.component<BoxCollider>()
		.member<glm::vec2>("offset")
		.member<glm::vec2>("size")
		.member<float>("dencity")
		.member<float>("friction")
		.member<float>("restitution");

	world.component<Velocity>()
		.member<float>("x")
		.member<float>("y");

	world.component<Direction>()
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

	world.observer<Physics, BoxCollider, Rigidbody, b2BodyId, Transform>("init colliders")
		.term_at(0).singleton()
		.term_at(2).filter()
		.term_at(3).filter()
		.term_at(4).filter()
		.event(flecs::OnSet)
		.each([](Physics& p, BoxCollider& b, Rigidbody& r, b2BodyId& id, Transform& t) {
			auto bodyDef = b2DefaultBodyDef();

			bodyDef.type          = r.type == Rigidbody::BodyType::Dynamic ? b2BodyType::b2_dynamicBody : b2BodyType::b2_staticBody;
			bodyDef.position      = b2Vec2{ (t.translation.x + b.size.x / 2) / PPM, (t.translation.y + b.size.y / 2) / PPM };
			bodyDef.rotation      = b2MakeRot(glm::radians(t.rotation.x));
			bodyDef.fixedRotation = r.fixedRotation;

			id = b2CreateBody(p.worldId, &bodyDef);

			auto polygon = b2MakeBox((b.size.x * t.scale.x) / 2.f / PPM, (b.size.y * t.scale.y ) / 2.f / PPM);

			auto shapeDef = b2DefaultShapeDef();

			shapeDef.density              = b.dencity;
			shapeDef.material.friction    = b.friction;
			shapeDef.material.restitution = b.restitution;

			b2CreatePolygonShape(id, &shapeDef, &polygon);
		});

	world.observer<Rigidbody, b2BodyId>()
		.term_at(1).filter()
		.event(flecs::OnSet)
		.each([](Rigidbody& r, b2BodyId& id) {
			if (id.index1 != 0) {
				b2Body_SetType(id, r.type == Rigidbody::BodyType::Dynamic ? b2BodyType::b2_dynamicBody : b2BodyType::b2_staticBody);
				b2Body_SetFixedRotation(id, r.fixedRotation);
			}
		});

	world.observer<b2BodyId>()
		.event(flecs::OnRemove)
		.each([](b2BodyId& id) {
			b2DestroyBody(id);
		});

	world.system<Direction, Velocity, b2BodyId>()
		.kind(Phases::Update)
		.each([](Direction& d, Velocity& v, b2BodyId& id) {
			b2Body_SetLinearVelocity(id, { d.x * v.x / PPM, d.y * v.y / PPM });
		});

	world.system<Transform, const b2BodyId, const BoxCollider>()
		.kind(Phases::Update)
		.each([](flecs::entity e, Transform& t, const b2BodyId& id, const BoxCollider& b) {
			const auto& position = b2Body_GetPosition(id);

			t.translation.x = position.x * PPM - b.size.x / 2.f;
			t.translation.y = position.y * PPM - b.size.y / 2.f;
			t.rotation.x    = glm::degrees(b2Rot_GetAngle(b2Body_GetRotation(id)));
		});

	world.add<Physics>();
}
