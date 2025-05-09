#pragma once

#include "raylib.h"
#include "flecs.h"
#include "glm.hpp"

#include <string>
#include <vector>

namespace ps {
	inline static constexpr const char* UI_ROO_ID = "uiRoot";

	struct RootNode {};
	struct Node {
		glm::vec2 pos;
		glm::vec2 size;
		glm::mat4 matrix;
	};

	struct ImageRenderData {
		Texture2D texture;
		Rectangle source;
		Rectangle dest;
		float rotation;
		Color color;
	};

	struct PrimitiveRenderData {
		Rectangle rec;
		float rotation;
		Color color;
	};

	struct TextRenderData {
		Font font;
		const char *text;
		Vector2 position;
		float rotation;
		float fontSize;
		float spacing;
		Color color;
	};

	using UiRenderData = std::variant<ImageRenderData, PrimitiveRenderData, TextRenderData>;

	struct UiRenderCommand {
		int sortIndex;
		UiRenderData renderData;
	};

	struct UiRenderCommandDispatcher {
		void operator()(const ImageRenderData& data) {
			DrawTexturePro(data.texture, data.source, data.dest, {}, data.rotation, data.color);
		}
		void operator()(const PrimitiveRenderData& data) {
			DrawRectanglePro(data.rec, {}, data.rotation, data.color);
		}
		void operator()(const TextRenderData& data) {
			DrawTextPro(data.font, data.text, data.position, {}, data.rotation, data.fontSize, data.spacing, data.color);
		}
	};

	struct UiRenderQueue {
		std::vector<UiRenderCommand> renderCommands;
	};

	struct Anchor : public glm::vec2 {};
	struct Pivot : public glm::vec2 {};
	struct BackgroundColor : public Color {
		BackgroundColor() : Color(WHITE) {}
		BackgroundColor(Color color) : Color(color) {}
	};

	struct Primitive {
		glm::vec2 size;
	};

	struct Interaction {
		enum class eType {
			None,
			Hovered,
			Clicked
		};

		eType type;
	};

	struct Text {
		std::string string;
		float fontSize;
		float spacing;
	};

	struct Image {
		std::string path;
		Texture2D texture;
		std::optional<Rectangle> part;
	};

	struct Button {
		glm::vec2 size;
		Color defaultColor;
		Color hoverColor;
		Color clickColor;
	};

	struct UiModule {
		UiModule(flecs::world& world);
	};
}
