#include "module.h"

#include "ecsModule/inputModule/module.h"
#include "ecsModule/physicsModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"
#include "raylib.h"
#include "spdlog/spdlog.h"

#include "gtx/compatibility.hpp"

#include <random>

using namespace ps;

PlayerModule::PlayerModule(flecs::world& world) {
	world.module<PlayerModule>();

	world.import<InputModule>();
	world.import<TransformModule>();
	world.import<PhysicsModule>();
	world.import<RenderModule>();
	world.import<CameraModule>();

	world.component<ePlayerState>().add(flecs::Union);
	world.component<DashData>();
	world.component<CameraTarget>().add(flecs::Exclusive);
	world.component<Player>()
		.add(flecs::With, world.component<Direction>())
		.add(flecs::With, world.component<Velocity>())
		.add(flecs::Exclusive);

	world.component<ePlayerState>()
		.constant("Idle", ePlayerState::Idle)
		.constant("Moving", ePlayerState::Moving)
		.constant("Dashing", ePlayerState::Dashing);

	world.observer<DashData, Velocity>()
		.with<Velocity>().filter()
		.event(flecs::OnSet)
		.each([](flecs::entity e, DashData& d, Velocity& v) {
			const auto speed = d.distance / d.duration;

			d.startVelocity = v;
			v.x = speed;
			v.y = speed;

			e.add(ePlayerState::Dashing);
		});

	world.observer<DashData, Velocity>()
		.with<Velocity>().filter()
		.event(flecs::OnRemove)
		.each([](flecs::entity e, DashData& d, Velocity& v) {
			v = d.startVelocity;

			e.add(ePlayerState::Idle);
		});

	world.system<Input, Velocity, Direction, Player>()
		.term_at(0).singleton()
		.without(ePlayerState::Dashing)
		.kind(Phases::Update)
		.each([](flecs::entity e, Input& i, Velocity& v, Direction& d, const Player& p) {
			d = { 0.f, 0.f };

			if (i.keys[Key::A].remain) {
				d.x = -1.f;
			}
			if (i.keys[Key::D].remain) {
				d.x = 1.f;
			}
			if (i.keys[Key::W].remain) {
				d.y = -1.f;
			}
			if (i.keys[Key::S].remain) {
				d.y = 1.f;
			}

			if (i.keys[Key::Space].pressed) {
				e.set<DashData>({
					.distance = 300.f,
					.duration = .1f
				});
			}

			if (d.x != 0 || d.y != 0) {
				d = glm::normalize(d);

				e.add(ePlayerState::Moving);
			} else {
				e.add(ePlayerState::Idle);
			}
		});

	world.system<Time, DashData>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](flecs::entity e, Time& t, DashData& d) {
			d.timer += t.deltaTime;

			if (d.timer >= d.duration) {
				e.remove<DashData>();
			}
		});

	world.system<const Time, Transform, const Velocity, const Transform>()
		.term_at(0).singleton()
		.term_at(1).src(CameraModule::EcsCamera)
		.term_at(2).src(CameraModule::EcsCamera)
		.with<CameraTarget>()
		.kind(Phases::Update)
		.each([](const Time& time, Transform& ct, const Velocity& cv, const Transform& t) {
			ct.translation = glm::lerp(ct.translation, t.translation, cv.x * time.deltaTime);
		});
}
