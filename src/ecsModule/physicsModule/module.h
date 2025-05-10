#pragma once

#include "box2d/id.h"
#include "vec2.hpp"
#include "flecs.h"

namespace ps {
	struct Physics {
		b2WorldId worldId{ b2_nullWorldId };
		glm::vec2 gravity{ 0.f, 0.f };
		int subStepCount{ 4 };
	};

	struct Rigidbody {
		enum class BodyType { 
			Static = 0,
			Dynamic,
			Kinematic,
		};

		BodyType type = BodyType::Static;
		bool fixedRotation = true;

		b2BodyId bodyId;
	};

	struct BoxCollider {
		glm::vec2 offset = { 0.f, 0.f };
		glm::vec2 size = { 0.f, 0.f };

		float dencity = 1.f;
		float friction = 0.f;
		float restitution = 0.f;
		float restitutionThreshold = 0.f;
	};

	struct Velocity {
		Velocity() = default;
		Velocity(glm::vec2 vec) : x(vec.x), y(vec.y) {}
		Velocity(float x, float y) : x(x), y(y) {}

		operator glm::vec2 () const {
			return { x, y };
		}

		float x;
		float y;
	};

	struct PhysicsModule {
		PhysicsModule(flecs::world& world);
	};
}
