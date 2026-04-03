#pragma once

#include "SDL3/SDL.h"
#include "flecs.h"
#include "texture.h"
#include "color.h"
#include "texture_atlas.h"

#include <memory>
#include <set>
#include <vector>

namespace ps {
	struct UiTreeChanged {};

	struct CustomNodeIndex {
		size_t value;
	};

	struct NodeVector {
		std::vector<flecs::entity> sorted_nodes;
	};

	struct Node {
		glm::vec2 size{ 0.f, 0.f };
		size_t stack_index = 0;
	};

	enum class Interaction {
		None,
		Hovered,
		Clicked,
	};

	enum class FocusStrategy {
		Block,
		Pass,
	};

	struct Button {
		bool hovered = false;
	};

	struct Image {
		std::shared_ptr<Texture> texture;
		std::optional<TextureAtlas> texture_atlas;
		Color color;
	};

	struct Text : public std::string {
		using std::string::string;

		Text(const std::string& string) : std::string(string) {}
	};

	struct Composite {
		enum class Element : std::uint8_t {
			TopLeft,
			Top,
			TopRight,
			Left,
			Middle,
			Right,
			BotLeft,
			Bot,
			BotRight,
		};

		static Composite create_3_v() {
			return {{
				0b00000'010,
				0b00000'010,
				0b00000'010
			}};
		}

		static Composite create_3_h() {
			return {{
				0b00000'000,
				0b00000'111,
				0b00000'000
			}};
		}

		static Composite create_9() {
			return {{
				0b00000'111,
				0b00000'111,
				0b00000'111
			}};
		}

		std::array<std::uint8_t, 3> structure;
	};

	struct BackgroundColor : public Color {
		using Color::Color;

		BackgroundColor(const Color& c) : Color(c) {}
	};

	struct UiModule {
		UiModule(flecs::world& world);
	};
}
