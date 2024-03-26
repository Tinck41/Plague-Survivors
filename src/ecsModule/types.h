#pragma once

#include "flecs.h"

namespace ps::ecsModule {
	enum class Phases {
		HandleInput,
		Update,
		Clear,
		Render,
		Display,
	};
}
