#include "module.h"

#include "components.h"
#include "ecsModule/text_render_module/module.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/textModule/module.h"
#include "font.h"

using namespace ps;

UiRenderModule::UiRenderModule(flecs::world& world) {
	world.module<UiRenderModule>();

	world.import<UiModule>();
	world.import<TextRenderModule>();

	text_engine = world.get<TextRenderModule>().engine;

	world.observer<Text, TextData, TextFont>()
		.event(flecs::OnSet)
		.each([&](flecs::entity e, Text& text, TextData& data, TextFont& font){
			if (!font.handle) {
				return;
			}

			if (!data.ttf_data) {
				data.ttf_data = TTF_CreateText(text_engine, *font.handle, text.c_str(), text.size());
			}
			else {
				TTF_SetTextString(data.ttf_data, text.c_str(), text.size());
			}

			TTF_GetTextSize(data.ttf_data, &data.size.x, &data.size.y);

			data.size = glm::vec2(data.size) * (font.size / font.handle->get_size());
		});

	world.observer<Text, TextData>()
		.event(flecs::OnRemove)
		.each([](flecs::entity e, Text& text, TextData& data){
			TTF_DestroyText(data.ttf_data);
			data.size = { 0, 0 };
		});

	world.component<ExtractedNodes>();
	world.component<ExtractedTextNodes>();
}

glm::mat4 ps::extract_ui_view(const Camera& camera, const GlobalTransform& transfrom) {
	return camera.projection;
}
