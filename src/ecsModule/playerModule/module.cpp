#include "module.h"

#include "ecsModule/inputModule/module.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"

using namespace ps;

PlayerModule::PlayerModule(flecs::world& world) {
	world.module<PlayerModule>();

	world.import<InputModule>();
	world.import<TransformModule>();
	world.import<PhysicsModule>();
	world.import<RenderModule>();
	world.import<CameraModule>();

	world.component<Player>().add(flecs::Exclusive);

	world.system<Input, Velocity, Player>()
		.kind(Phases::Update)
		.term_at(0).singleton()
		.each([](flecs::entity entity, Input& i, Velocity& v, const Player& p) {
			glm::vec2 direction { 0.f, 0.f };

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
				normalized = glm::normalize(direction);
			}

			constexpr auto speed = 700.f;
			v = glm::vec2{ speed * normalized.x, speed * normalized.y };
		});
}
