#pragma once

#include "SFML/System/Vector2.hpp"

namespace ps {
	enum class Phases {
		OnStart,
		HandleInput,
		PreUpdate,
		Update,
		PostUpdate,
		Clear,
		Render,
		RenderUI,
		Display,
	};
}
