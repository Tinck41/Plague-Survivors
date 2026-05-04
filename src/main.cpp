#include "core/application.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/default_modules.h"
#include "ecsModule/editor_module/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/ui_render_module_new/components.h"
#include "ecsModule/ui_render_module_new/module.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/utils.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/ui_render_module_new/text_helpers.h"
#include "ecsModule/ui_render_module_new/node_helpers.h"
#include "ecsModule/windowModule/components.h"
#include "spdlog/spdlog.h"
#include "texture_atlas.h"
#include <algorithm>
#include "ecsModule/render_module/module.h"
#include "ecsModule/sprite_render_module/module.h"
#include "ecsModule/text_render_module/module.h"
#include "utils/sdl.h"

void default_init(flecs::world& world) {
	using namespace ps;

	auto camera = world.entity("camera")
		.set<ps::Transform>({ .translation = { 0.f, 0.f, 0.f } })
		.set<ps::RenderLayers>({ RenderLayers::layer(2) | RenderLayers::layer(1) })
		.add<ps::Camera>();

	{
		auto transparent_2d = world.entity("transparent_2d");
		auto transparent_ui = world.entity("transparent_ui");

		{
			auto sprite_phase = world.entity("sprite_phase").child_of(transparent_2d);
			auto text_phase = world.entity("text_phase").child_of(transparent_2d);

			sprite_phase.set(create_sprite_material(world));
			sprite_phase.set(create_sprite_context(world));

			text_phase.set(create_text_material(world));
			text_phase.set(create_text_context(world));

			sprite_phase.set(create_sprite_extractor(world, sprite_phase));
			sprite_phase.set(create_sprite_uploader());

			text_phase.set(create_text_extractor(world, text_phase));
			text_phase.set(create_text_uploader());

			sprite_phase.add<ExtractedSprites>();
			text_phase.add<ExtractedText2ds>();

			transparent_2d.set(create_default_sorter());
			transparent_2d.set(create_default_renderer());

			transparent_2d.set(RenderPhase{
				.extractors = { sprite_phase, text_phase },
				.sorters = { transparent_2d },
				.uploaders = { sprite_phase, text_phase },
				.renderers = { transparent_2d },
			});
		}

		{
			auto node_helper = world.entity("node_helper").child_of(transparent_ui);
			auto text_node_helper = world.entity("text_node_helper").child_of(transparent_ui);
			auto image_extractor = world.entity("image_extractor").child_of(node_helper);
			auto color_extractor = world.entity("color_extractor").child_of(node_helper);

			node_helper.add<ExtractedNodes>();
			text_node_helper.add<ExtractedTextNodes>();

			node_helper.set(create_node_context(world));
			text_node_helper.set(create_text_context(world));

			transparent_ui.set(create_default_sorter());
			transparent_ui.set(create_default_renderer());

			node_helper.set(create_node_material(world));
			text_node_helper.set(create_text_material(world));

			image_extractor.set(create_image_node_extractor(world, node_helper));
			color_extractor.set(create_color_node_extractor(world, node_helper));
			text_node_helper.set(create_text_node_extractor(world, text_node_helper));

			node_helper.set(create_node_uploader());
			text_node_helper.set(create_text_node_uploader());

			transparent_ui.set(RenderPhase{
				.extractors = { image_extractor, color_extractor, text_node_helper },
				.sorters = { transparent_ui },
				.uploaders = { node_helper, text_node_helper },
				.renderers = { transparent_ui },
				.extract_view_callback = &extract_ui_view,
			});
		}

		camera.set(RenderPhases{ transparent_2d, transparent_ui });
	}
}

int main() {
	ps::Application::create()
		.add_module<ps::DefaultModules>()
		.add_module<ps::EditorModule>()
		.add_system(ps::Phases::OnStart, &default_init)
		.run();
}

