#pragma once

#include "flecs.h"

namespace ps::ecsModule {
	struct RendererComponent;
}

namespace ps::ecsModule::ui {
	class UiSystem {
	public:
		static void create();
	private:
		static void drawCall(flecs::entity entity, RendererComponent& renderer);
		UiSystem() = default;
	};
}
