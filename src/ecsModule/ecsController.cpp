#include "ecsController.h"

#include "ecsModule/uiModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/playerModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "common.h"

using namespace ps;
using namespace ecsModule;

std::unique_ptr<EcsController> EcsController::create() {
	return std::unique_ptr<EcsController>(new EcsController);
}

void EcsController::init() {
//	initSystemPhases();
//
//	m_world.import<TimeModule>();
//	m_world.import<TransformModule>();
//	m_world.import<InputModule>();
//	m_world.import<UiModule>();
//	m_world.import<PhysicsModule>();
//	m_world.import<CameraModule>();
//	m_world.import<SpriteModule>();
//	m_world.import<PlayerModule>();
//
//	auto player = m_world.entity("player")
//	.add<Player>()
//	.add<HandleInputState>()
//	.insert([](Transform& t, Velocity& v, Sprite& s) {
//		t.translation = { 50.f, 50.f };
//		s.sprite.setSize({ 50.f, 50.f });
//		//s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
//		s.color = sf::Color::Green;
//		s.sprite.setFillColor(s.color);
//		v = { 300.f, 300.f };
//	})
//	.set<BoxCollider>({ .size = { 50.f, 50.f } })
//	.set<Rigidbody>({ .Type = Rigidbody::BodyType::Dynamic });
//
//	m_world.entity("turret")
//		.insert([](Transform& t, Sprite& s) {
//			t.translation = { 50.f, 0.f };
//			s.sprite.setSize({ 100.f, 50.f });
//			s.color = sf::Color::Blue;
//			s.sprite.setFillColor(s.color);
//		})
//		.child_of(player);
//
//	auto camera = m_world.get_ref<Camera>();
//	camera->target = player;
//
//	m_world.entity()
//	.insert([](Transform& t, Sprite& s) {
//		t.translation = { 300.f, 300.f };
//		s.sprite.setSize({ 150.f, 150.f });
//		//s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
//		s.color = sf::Color::Red;
//		s.sprite.setFillColor(s.color);
//	})
//	.set<BoxCollider>({ .size = { 150.f, 150.f } })
//	.set<Rigidbody>({ .Type = Rigidbody::BodyType::Static });
//	m_world.entity("obstacle")
//	.insert([](Transform& t, Sprite& s) {
//		t.translation = { 600.f, 50.f };
//		s.sprite.setSize({ 50.f, 50.f });
//		//s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
//		s.color = sf::Color::Red;
//		s.sprite.setFillColor(s.color);
//	})
//	.set<BoxCollider>({ .size = { 50.f, 50.f } })
//	.set<Rigidbody>({ .Type = Rigidbody::BodyType::Static });
}

void EcsController::initSystemPhases() {
	m_world.entity(Phases::HandleInput)
		.add(flecs::Phase)
		.depends_on(flecs::PostLoad);

	m_world.entity(Phases::Update)
		.add(flecs::Phase)
		.depends_on(flecs::OnUpdate);

	m_world.entity(Phases::PostUpdate)
		.add(flecs::Phase)
		.depends_on(Phases::Update);

	m_world.entity(Phases::Clear)
		.add(flecs::Phase)
		.depends_on(Phases::PostUpdate);

	m_world.entity(Phases::Render)
		.add(flecs::Phase)
		.depends_on(Phases::Clear);

	m_world.entity(Phases::Display)
		.add(flecs::Phase)
		.depends_on(Phases::Render);
}

flecs::world& EcsController::getWorld() {
	return m_world;
}
