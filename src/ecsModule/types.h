#pragma once

namespace ps::ecsModule {
	enum class Phases {
		HandleInput,
		Update,
		Clear,
		Render,
		Display,
	};
}
