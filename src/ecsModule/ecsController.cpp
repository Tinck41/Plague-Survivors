#include "ecsController.h"

#include "SFML/System/Vector2.hpp"
#include "ecsModule/components/boxColliderComponent.h"
#include "ecsModule/components/cameraComponent.h"
#include "ecsModule/components/inputComponent.h"
#include "ecsModule/components/playerComponent.h"
#include "ecsModule/components/rigidbodyComponent.h"
#include "ecsModule/components/spriteComponent.h"
#include "ecsModule/components/velocityComponent.h"
#include "ecsModule/components/transformComponent.h"
#include "ecsModule/systems/inputSystem.h"
#include "ecsModule/systems/physicsSystem.h"
#include "ecsModule/systems/timeSystem.h"
#include "ecsModule/systems/cameraSystem.h"
#include "spdlog/spdlog.h"
#include "systems/renderSystem.h"
#include "box2d/b2_body.h"
#include "types.h"
#include <iostream>

using namespace ps;
using namespace ecsModule;

std::unique_ptr<EcsController> EcsController::create() {
	return std::unique_ptr<EcsController>(new EcsController);
}

void EcsController::init() {
	initSystemPhases();
	initSystems();

	m_world.system<InputComponent, RigidbodyComponent, VelocityComponent, PlayerComponent>()
	.kind(flecs::OnUpdate)
	.term_at(1).singleton()
	.each([](flecs::iter& it, size_t,InputComponent& i, RigidbodyComponent& r, const VelocityComponent& v, const PlayerComponent& p) {
		sf::Vector2f direction { 0.f, 0.f };
		if (i.keys[Key::A].remain) {
			direction.x = -1.f;
		}
		if (i.keys[Key::D].remain) {
			direction.x = 1.f;
		}
		if (i.keys[Key::W].remain) {
			direction.y = -1.f;
		}
		if (i.keys[Key::S].remain) {
			direction.y = 1.f;
		}
		auto normalized = direction;
		if (direction.x != 0 || direction.y != 0) {
			normalized = direction.normalized();
		}
		const auto velocity = b2Vec2{ v.velocity.x / 30.f * normalized.x, v.velocity.y / 30.f * normalized.y };
		auto body = (b2Body*)r.body;
		body->SetLinearVelocity(velocity);
	});

	m_world.system<SpriteComponent, TransformComponent>()
	.kind(Phases::Update)
	.each([](SpriteComponent& s, const TransformComponent& t) {
		s.sprite.setPosition({ t.translation.x, t.translation.y });
	});

	auto player = m_world.entity("player")
	.add<PlayerComponent>()
	.set([](TransformComponent& t, VelocityComponent& v, SpriteComponent& s) {
		t.translation = { 50.f, 50.f };
		s.sprite.setSize({ 50.f, 50.f });
		s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
		s.color = sf::Color::Green;
		s.sprite.setFillColor(s.color);
		v.velocity = { 500.f, 500.f };
	})
	.set([](RigidbodyComponent& r, BoxColliderComponent& b) {
		b.size = { 50.f, 50.f };
		r.Type = RigidbodyComponent::BodyType::Dynamic;
	});

	auto camera = m_world.get_ref<CameraComponent>();
	camera->target = player;

	m_world.entity()
	.set([](TransformComponent& t, SpriteComponent& s) {
		t.translation = { 300.f, 300.f };
		s.sprite.setSize({ 150.f, 150.f });
		s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
		s.color = sf::Color::Red;
		s.sprite.setFillColor(s.color);
	})
	.set([](RigidbodyComponent& r, BoxColliderComponent& b) {
		b.size = { 150.f, 150.f };
		r.Type = RigidbodyComponent::BodyType::Static;
	});
	m_world.entity("obstacle")
	.set([](TransformComponent& t, SpriteComponent& s) {
		t.translation = { 600.f, 50.f };
		s.sprite.setSize({ 50.f, 50.f });
		s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
		s.color = sf::Color::Red;
		s.sprite.setFillColor(s.color);
	})
	.set([](RigidbodyComponent& r, BoxColliderComponent& b) {
		b.size = { 50.f, 50.f };
		r.Type = RigidbodyComponent::BodyType::Static;
	});
}

void EcsController::initSystemPhases() {
	m_world.entity(Phases::HandleInput)
		.add(flecs::Phase)
		.depends_on(flecs::PostLoad);

	m_world.entity(Phases::Update)
		.add(flecs::Phase)
		.depends_on(flecs::OnUpdate);

	m_world.entity(Phases::Clear)
		.add(flecs::Phase)
		.depends_on(flecs::PostUpdate);

	m_world.entity(Phases::Render)
		.add(flecs::Phase)
		.depends_on(Phases::Clear);

	m_world.entity(Phases::Display)
		.add(flecs::Phase)
		.depends_on(Phases::Render);
}

void EcsController::initSystems() {
	RenderSystem::create();
	TimeSystem::create();
	PhysicsSystem::create();
	InputSystem::create();
	CameraSystem::create();
}

flecs::world& EcsController::getWorld() {
	return m_world;
}
