#pragma once

namespace ps {
	enum class Phases {
		OnStart,
		HandleInput,
		PreUpdate,
		Update,
		PostUpdate,
		CalcTransform, // TODO: need rethinck system ordering
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
