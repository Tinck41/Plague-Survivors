#pragma once

#include "ecsModule/transformModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "flecs.h"
#include "glm.hpp"

namespace se {
	struct UiRenderModule {
		UiRenderModule(flecs::world& world);

		TTF_TextEngine* text_engine;
	};

	glm::mat4 extract_ui_view(const Camera& camera, const GlobalTransform& transfrom);
}
