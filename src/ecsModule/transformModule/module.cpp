#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/sceneModule/module.h"

using namespace ps;

TransformModule::TransformModule(flecs::world& world) {
	world.module<TransformModule>();

	world.component<Vec2f>()
		.member<float>("x")
		.member<float>("y");

	world.component<Vec3f>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z");

	world.component<Transform>()
		.member<Vec3f>("translation")
		.member<Vec3f>("scale")
		.member<float>("rotation");

	world.component<GlobalTransform>()
		.member<Vec3f>("translation")
		.member<Vec3f>("scale")
		.member<float>("rotation");

	world.observer<Transform>()
		.term_at(0).self().up()
		.event(flecs::OnAdd)
		.event(flecs::OnSet)
		.each([](flecs::entity e, Transform& p) {
			e.add<GlobalTransform>();
		});

	world.system<GlobalTransform*, GlobalTransform, Transform>()
		.term_at(0).parent().cascade()
		.kind(Phases::Update)
		.each([](flecs::entity e, GlobalTransform* parentGlobal, GlobalTransform& childGlobal, Transform& childLocal) {
			childGlobal.translation = childLocal.translation;
			childGlobal.scale.x     = childLocal.scale.x;
			childGlobal.scale.y     = childLocal.scale.y;
			childGlobal.rotation    = childLocal.rotation;

			if (parentGlobal) {
				childGlobal.translation += parentGlobal->translation;
				childGlobal.scale       += parentGlobal->scale;
				childGlobal.rotation    += parentGlobal->rotation;
			}
		});
}
