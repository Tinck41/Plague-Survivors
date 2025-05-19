#pragma once

#include "raylib.h"
#include "flecs.h"
#include "vec2.hpp"

#include <memory>
#include <variant>
#include <optional>
#include <vector>
#include <string>

namespace ps {
	struct Window {
		std::string title;
		int width;
		int height;
	};

	struct Resized {};

	struct SpriteRenderData {
		Texture2D texture;
		Rectangle source;
		Rectangle dest;
		Vector2 origin;
		float rotation;
		Color color;
	};

	struct RectRenderData {
		Rectangle rec;
		float rotation;
		Color color;
	};

	struct CircleRenderData {
		glm::vec2 center;
		float radius;
		Color color;
	};

	using RenderData = std::variant<SpriteRenderData, RectRenderData, CircleRenderData>;

	struct RenderCommand {
		int sortIndex;
		RenderData renderData;
	};

	struct RenderCommandDispatcher {
		void operator()(const SpriteRenderData& data) {
			DrawTexturePro(data.texture, data.source, data.dest, data.origin, data.rotation, data.color);
		}
		void operator()(const RectRenderData& data) {
			DrawRectanglePro(data.rec, {}, data.rotation, data.color);
		}
		void operator()(const CircleRenderData& data) {
			DrawCircle(data.center.x, data.center.y, data.radius, data.color);
		}
	};

	struct RenderQueue {
		std::vector<RenderCommand> renderCommands;
	};

	struct Sprite {
		std::shared_ptr<Texture2D> texture;
		Color color = WHITE;
		std::optional<Rectangle> source;
		Vector2 origin;
	};

	struct Circle {
		float radius;
		Color color = WHITE;
	};

	struct Rect {
		glm::vec2 size;
		Color color = WHITE;
	};

	struct RenderModule {
		RenderModule(flecs::world& world);
	};
}
