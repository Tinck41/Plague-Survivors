#pragma once

#include "flecs.h"

namespace ps::ecsModule {
	class CameraSystem {
	public:
		static void create();
	private:
		CameraSystem() = default;
	};
}
