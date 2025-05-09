#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/transformModule/module.h"
#include "raylib.h"
#include "resourceManager.h"

using namespace ps;

RenderModule::RenderModule(flecs::world& world) {
	world.module<RenderModule>();

	world.import<TransformModule>();

	world.component<Color>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a");

	world.component<Sprite>()
		.member<Color>("color")
		.add(flecs::With, world.component<Transform>());
	world.component<Rectangle>().add(flecs::With, world.component<Transform>());
	world.component<Circle>().add(flecs::With, world.component<Transform>());
	world.component<Window>();

	world.observer<Window>()
		.event(flecs::OnSet)
		.each([](Window& win) {
			InitWindow(win.width, win.height, win.title.c_str());
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
					.rec      = { t.translation.x, t.translation.y, r.size.x, r.size.y },
					.rotation = 0.f, // TODO
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
					.dest     = { t.translation.x, t.translation.y, source.width, source.height },
					.rotation = 0.f, // TODO
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

	world.system<Window>("BeginDrawing")
		.term_at(0).singleton()
		.kind(Phases::Clear)
		.each([](Window& w) {
			BeginDrawing();
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

	world.system<Window>("EndDrawing")
		.term_at(0).singleton()
		.kind(Phases::Display)
		.each([](Window& w) {
			EndDrawing();
		});

	world.set<Window>({
		"Plague: Survivors",
		600, 600
	});
}
