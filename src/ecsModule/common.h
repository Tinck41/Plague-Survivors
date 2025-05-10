#pragma once

namespace ps {
	enum class Phases {
		OnStart,
		HandleInput,
		PreUpdate,
		Update,
		PostUpdate,
		Clear,
		PreRender,
		Render,
		PostRender,
		RenderUI,
		Display,
	};
}
