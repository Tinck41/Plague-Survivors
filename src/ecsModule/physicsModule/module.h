#pragma once

#include "vec2.hpp"
#include "flecs.h"

namespace ps {
	struct Rigidbody {
		enum class BodyType { 
			Static = 0,
			Dynamic,
			Kinematic,
		};

		BodyType Type = BodyType::Static;
		bool fixedRotation = true;

		void* body = nullptr;
	};

	struct BoxCollider {
		glm::vec2 offset = { 0.f, 0.f };
		glm::vec2 size = { 0.f, 0.f };

		float dencity = 1.f;
		float friction = 0.f;
		float restitution = 0.f;
		float restitutionThreshold = 0.f;

		void* fixture = nullptr;
	};

	using Velocity = glm::vec2;

	struct PhysicsModule {
		PhysicsModule(flecs::world& world);
	};
}
