#pragma once

#include "box2d/b2_world.h"

namespace ps::ecsModule {
	struct PhysicsComponent {
		b2World* world = nullptr;

		const int32_t velocityInterations = 6;
		const int32_t positionInterations = 2;

		PhysicsComponent() {
			world = new b2World({ 0.f, 0.f });
		}

		~PhysicsComponent() {
			delete world;
			world = nullptr;
		}

		PhysicsComponent(PhysicsComponent&& other) {
			world = std::move(other.world);
			other.world = nullptr;
		}

		PhysicsComponent(PhysicsComponent& other) = delete;

		PhysicsComponent& operator=(PhysicsComponent&& other) {
			auto p = std::move(other.world);
			other.world = nullptr;
			return *this;
		}
	};
}
