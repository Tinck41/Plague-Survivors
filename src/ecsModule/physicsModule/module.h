#pragma once

#include "SFML/System/Vector2.hpp"
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
		sf::Vector2f offset = { 0.f, 0.f };
		sf::Vector2f size = { 0.f, 0.f };

		float dencity = 1.f;
		float friction = 0.f;
		float restitution = 0.f;
		float restitutionThreshold = 0.f;

		void* fixture = nullptr;
	};

	using Velocity = sf::Vector2f;

	struct PhysicsModule {
		PhysicsModule(flecs::world& world);
	};
}
