#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/transformModule/module.h"
#include "raylib.h"
#include "resourceManager.h"

#include <algorithm>

using namespace ps;

RenderModule::RenderModule(flecs::world& world) {
	world.module<RenderModule>();

	world.import<TransformModule>();

	world.component<Sprite>()
		.member<Color>("color")
		.add(flecs::With, world.component<Transform>());
	world.component<Rect>().add(flecs::With, world.component<Transform>());
	world.component<Circle>().add(flecs::With, world.component<Transform>());
	world.component<Window>().add(flecs::With, world.component<RenderTexture2D>());

	world.component<Color>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a");

	world.component<std::string>()
		.opaque(flecs::String)
			.serialize([](const flecs::serializer *s, const std::string *data) {
				const char *str = data->c_str();
				return s->value(flecs::String, &str);
			})
			.assign_string([](std::string* data, const char *value) {
				*data = value;
			});


	world.component<Window>()
		.member<std::string>("title")
		.member<int>("width")
		.member<int>("height");

	world.component<Rect>()
		.member<glm::vec2>("size")
		.member<Color>("color");

	world.observer<Window, RenderTexture2D>()
		.event(flecs::OnSet)
		.each([](Window& win, RenderTexture2D& renderTexture) {
			InitWindow(win.width, win.height, win.title.c_str());
			renderTexture = LoadRenderTexture(win.width, win.height);
		});

	world.observer<Window>()
		.event(flecs::OnRemove)
		.each([](Window& win) {
			CloseWindow();
		});

	world.system<RenderQueue, GlobalTransform, Rect>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](RenderQueue& queue, GlobalTransform& t, Rect& r) {
			queue.renderCommands.emplace_back(
				t.translation.z,
				RectRenderData{
					.rec      = { t.translation.x, t.translation.y, r.size.x * t.scale.x, r.size.y * t.scale.y },
					.rotation = t.rotation.x,
					.color    = r.color
				}
			);
		});

	world.system<RenderQueue, GlobalTransform, Sprite>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](RenderQueue& queue, GlobalTransform& t, Sprite& s) {
			const auto source = [s]() {
				if (s.source) {
					return s.source.value();
				}

				return Rectangle{ 0.f, 0.f, static_cast<float>(s.texture->width), static_cast<float>(s.texture->height) };
			}();

			queue.renderCommands.emplace_back(
				t.translation.z,
				SpriteRenderData{
					.texture  = *s.texture,
					.source   = source,
					.dest     = { t.translation.x, t.translation.y, source.width * t.scale.x, source.height * t.scale.x },
					.rotation = t.rotation.x,
					.color    = s.color,
				}
			);
		});

	world.system<RenderQueue, GlobalTransform, Circle>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.each([](RenderQueue& queue, GlobalTransform& t, Circle& c) {
			queue.renderCommands.emplace_back(
				t.translation.z,
				CircleRenderData{
					.center = t.translation,
					.radius = c.radius,
					.color  = c.color,
				}
			);
		});

	world.system<RenderTexture2D>("BeginDrawing")
		.kind(Phases::Clear)
		.each([](RenderTexture2D& texture) {
			BeginDrawing();
			//BeginTextureMode(texture);
			ClearBackground(BLACK);
		});

	world.system<RenderQueue>("RenderSystem")
		.term_at(0).singleton()
		.kind(Phases::Render)
		.each([](RenderQueue& queue) {
			std::ranges::sort(queue.renderCommands, [](const RenderCommand& lhs, const RenderCommand& rhs) {
				return lhs.sortIndex < rhs.sortIndex;
			});

			for (const auto& command : queue.renderCommands) {
				std::visit(RenderCommandDispatcher{}, command.renderData);
			}

			queue.renderCommands.clear();
		});

	world.system<Window, RenderTexture2D>("EndDrawing")
		.kind(Phases::Display)
		.each([](Window& w, RenderTexture2D& texture) {
			//const auto scale = std::max(
			//	std::floorf(
			//		std::min(
			//			w.width / static_cast<float>(texture.texture.width),
			//			w.height / static_cast<float>(texture.texture.height)
			//		)
			//	),
			//	1.f
			//);
			//EndTextureMode();
			//ClearBackground(BLACK);
			//DrawTexturePro(
			//	texture.texture,
			//	{ 0, 0, static_cast<float>(texture.texture.width), static_cast<float>(-texture.texture.height) },
			//	//{ (w.width - texture.texture.width) / 2.f, (w.height - texture.texture.height) / 2.f, texture.texture.width * scale, texture.texture.height * scale },
			//	{ 0, 0, texture.texture.width * scale, texture.texture.height * scale },
			//	{},
			//	0,
			//	WHITE
			//);
			EndDrawing();
		});

	world.set<RenderQueue>({});
}
