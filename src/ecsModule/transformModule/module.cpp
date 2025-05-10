#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/sceneModule/module.h"
#include <functional>
#include "ext/matrix_transform.hpp"
#include "gtc/quaternion.hpp"

using namespace ps;

TransformModule::TransformModule(flecs::world& world) {
	world.module<TransformModule>();

	world.component<Dirty>();
	world.component<glm::vec2>()
		.member<float>("x")
		.member<float>("y");

	world.component<glm::vec3>()
		.member<float>("x")
		.member<float>("y")
		.member<float>("z");

	world.component<Transform>()
		.member<glm::vec3>("translation")
		.member<glm::vec3>("scale")
		.member<float>("rotation")
		.add(flecs::With, world.component<GlobalTransform>());

	world.component<GlobalTransform>()
		.member<glm::vec3>("translation")
		.member<glm::vec3>("scale")
		.member<float>("rotation");

	world.observer<Transform>()
		.event(flecs::OnSet)
		.each([](flecs::entity e, Transform& t) {
			e.add<Dirty>();
		});

	world.system<Dirty>()
		.term_at(0).parent().cascade()
		.each([](flecs::entity e, Dirty) {
			e.add<Dirty>();
		});

	world.system<GlobalTransform*, GlobalTransform, Transform>()
		.term_at(0).parent().cascade()
		.kind(Phases::Update)
		.each([](flecs::entity e, GlobalTransform* parentGlobal, GlobalTransform& childGlobal, Transform& childLocal) {
			childLocal.matrix =
				glm::translate(glm::mat4{ 1.f }, childLocal.translation) *
				glm::mat4_cast(childLocal.rotation) *
				glm::scale(glm::mat4{ 1.f }, childLocal.scale);

			childGlobal.matrix = childLocal.matrix;

			if (parentGlobal) {
				childGlobal.matrix = parentGlobal->matrix * childLocal.matrix;
			}

			childGlobal.translation = glm::vec3{ childGlobal.matrix[3] };
			childGlobal.scale.x = glm::length(glm::vec3{ childGlobal.matrix[0] });
			childGlobal.scale.y = glm::length(glm::vec3{ childGlobal.matrix[1] });
			childGlobal.scale.z = glm::length(glm::vec3{ childGlobal.matrix[2] });
			childGlobal.rotation = glm::quat_cast(childGlobal.matrix);
		});

	world.system<Dirty>()
		.kind(Phases::PostUpdate)
		.each([](flecs::entity e, Dirty) {
			e.remove<Dirty>();
		});
}
