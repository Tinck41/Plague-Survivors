#pragma once 

#include "box2d/b2_world.h"
#include "utils/instanceBase.h"

#include <memory>

namespace ps::physics {
	class PhysicsController {
	public:
		[[nodiscard]] static std::unique_ptr<PhysicsController> create();

		void init();

		b2World* getWorld();
	private:
		PhysicsController() = default;

		std::unique_ptr<b2World> m_world;
	};

	using PhysicsControllerInstance = utils::InstanceBase<PhysicsController>;
}
