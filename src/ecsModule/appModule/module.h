#pragma once

#include "flecs.h"

namespace ps {
	struct Application {
		int targetFPS = 60;
		bool isVSyncEnabled = false;
	};

	struct AppModule {
		AppModule(flecs::world& world);
	};
}
