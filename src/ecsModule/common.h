#pragma once

namespace ps {
	enum class Phases {
		OnStart,
		HandleInput,
		PreUpdate,
		Update,
		PostUpdate,
		CollectRenderData,
		SortRenderData,
		PrepareRenderData,
		Clear,
		PreRender,
		Render,
		PostRender,
		RenderUI,
		Display,
	};
}
