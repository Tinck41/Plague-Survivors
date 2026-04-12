#pragma once

#include "flecs.h"
#include "ecsModule/render_module/render_phase.h"

namespace ps {
	RenderPhaseExtractor create_text_node_extractor(flecs::world& world, flecs::entity_t helper);
	RenderPhaseUploader create_text_node_uploader();
}

