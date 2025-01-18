#pragma once

#include "flecs.h"

namespace ps::ecsModule {
	struct RendererComponent;
}

namespace ps::ecsModule::ui {
    struct InteractionComponent;
}

namespace ps::ecsModule::ui {
	class UiSystem {
	public:
		static void create();
	private:
		static void drawCall(flecs::entity entity, RendererComponent& renderer);
        static bool mouseEvent(flecs::entity entity, RendererComponent& renderer, InteractionComponent& interaction, bool hovered);
		UiSystem() = default;
	};
}
