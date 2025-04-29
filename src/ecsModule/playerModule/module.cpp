#include "module.h"

#include "SFML/System/Vector2.hpp"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/sceneModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include <iostream>

using namespace ps;

PlayerModule::PlayerModule(flecs::world& world) {
	world.module<PlayerModule>();

	world.component<Player>().add(flecs::Exclusive);
	world.component<IdleState>();
	world.component<HandleInputState>();
	world.component<DashState>();

	world.system<Input, Velocity, Player>()
		.with<HandleInputState>()
		.kind(Phases::Update)
		.term_at(0).singleton()
		.each([](flecs::entity entity, Input& i, Velocity& v, const Player& p) {
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

			//if (i.keys[Key::Space].pressed && (direction.x != 0 || direction.y != 0)) {
			//	Dash dash;
			//	dash.direction = normalized;
			//	dash.distance = 150.f;
			//	dash.duration = 0.1f;
			//	entity.set(dash);
			//}

			constexpr auto speed = 500.f;
			v = sf::Vector2f{ speed * normalized.x, speed * normalized.y };
		});

	world.observer<Dash>()
		.event(flecs::OnSet)
		.each([](flecs::entity entity, Dash& dash) {
			entity.remove<HandleInputState>();
			entity.add<DashState>();

			auto v = entity.get_ref<Velocity>().get();
			auto speed = dash.distance / dash.duration;
			*v = sf::Vector2f{ speed * dash.direction.x, speed * dash.direction.y };
		});

	world.observer<Dash>()
		.event(flecs::OnRemove)
		.each([](flecs::iter& it, size_t size, Dash& dash) {
			auto entity = it.entity(size);
			entity.add<HandleInputState>();
			entity.remove<DashState>();

			//auto camera = it.world().singleton<Camera>();
			//camera.set(CameraShaking {
			//	.duration = 0.1f,
			//	.horizontalOffset = { -10.f, 10.f },
			//	.verticalOffset = { -10.f, 10.f }
			//});

			auto v = entity.get_ref<Velocity>().get();
			auto speed = 500.f;
			*v = sf::Vector2f{ speed, speed };
		});

	world.system<Dash, Velocity>()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t size, Dash& d, Velocity& v) {
			if (d.timer < d.duration) {
				d.timer += it.delta_time();
			} else {
				it.entity(size).remove<Dash>();
			}
		});


	auto player = world.entity("player")
	.add<Player>()
	.add<HandleInputState>()
	//.insert([](Transform& t, Velocity& v, Sprite& s) {
	//	t.translation = { 50.f, 50.f };
	//	s.sprite.setSize({ 50.f, 50.f });
	//	//s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
	//	s.color = sf::Color::Green;
	//	s.sprite.setFillColor(s.color);
	//	//v = { 0.f, 0.f };
	//	v = { 300.f, 300.f };
	//})
	.set<BoxCollider>({ .size = { 50.f, 50.f } })
	.set<Rigidbody>({ .Type = Rigidbody::BodyType::Dynamic });

	std::cout << player.path() << "\n";

	auto camera = world.get_ref<Camera>();
	camera->target = player;
	const auto offset = player.get<BoxCollider>()->size * 0.5f;
	camera->offset = offset;

	auto asd = world.lookup("player");

	world.entity()
	//.insert([](Transform& t, Sprite& s) {
	//	t.translation = { 300.f, 300.f };
	//	s.sprite.setSize({ 150.f, 150.f });
	//	//s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
	//	s.color = sf::Color::Red;
	//	s.sprite.setFillColor(s.color);
	//})
	.set<BoxCollider>({ .size = { 150.f, 150.f } })
	.set<Rigidbody>({ .Type = Rigidbody::BodyType::Static });

	world.entity("obstacle")
	//.insert([](Transform& t, Sprite& s) {
	//	t.translation = { 600.f, 50.f };
	//	s.sprite.setSize({ 50.f, 50.f });
	//	//s.sprite.setOrigin(s.sprite.getSize() * 0.5f);
	//	s.color = sf::Color::Red;
	//	s.sprite.setFillColor(s.color);
	//})
	.set<BoxCollider>({ .size = { 50.f, 50.f } })
	.set<Rigidbody>({ .Type = Rigidbody::BodyType::Static });
}
