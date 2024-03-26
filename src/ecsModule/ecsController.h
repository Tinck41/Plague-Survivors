#pragma once

#include <memory>

#include "utils/instanceBase.h"
#include "flecs.h"

namespace ps::ecsModule {
	class EcsController {
	public:
		[[nodiscard]] static std::unique_ptr<EcsController> create();

		void init();

		flecs::world& getWorld();
	private:
		EcsController() = default;

		void initSystemPhases();
		void initSystems();

		flecs::world m_world;
	};

	using EcsControllerInstance = ps::utils::InstanceBase<EcsController>;
}
