#pragma once

#include "SDL3/SDL.h"
#include "flecs.h"
#include "texture.h"
#include "color.h"
#include "texture_atlas.h"

#include <memory>
#include <set>
#include <variant>
#include <vector>

namespace ps {
	struct UiTreeChanged {};

	struct CustomNodeIndex {
		size_t value;
	};

	struct Node {
		enum class GrowDirection : std::uint8_t {
			Horizontal,
			Vertical,
		};

		enum class Overflow : std::uint8_t {
			Visible,
			Clip,
			Scroll,
		};

		struct Fixed {
			float value;
		};

		struct Fit {
			std::optional<float> min;
			std::optional<float> max;
		};

		struct Grow {
			std::optional<float> min;
			std::optional<float> max;
		};

		using SizePolicy = std::variant<Fixed, Fit, Grow>;

		std::pair<SizePolicy, SizePolicy> sizing_policy = { Fit{}, Fit{} };
		std::pair<Overflow, Overflow> overflow = { Overflow::Visible, Overflow::Visible };
		std::pair<std::optional<float>, std::optional<float>> self_alignment;

		glm::vec2 pos;
		glm::vec2 size;
		glm::vec2 child_alignment;
		glm::vec2 child_gap;
		glm::vec2 offsets;
		glm::vec4 margin; // l, r, t, b
		glm::vec4 padding; // l, r, t, b

		GrowDirection grow_direction = GrowDirection::Horizontal;

		bool display = true;
		bool absolute = false;

		float border_radius = 0.f;
		float border_width = 0.f;
	};

	struct NodeIndex {
		size_t dfs;
		size_t bfs;
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

	struct BorderColor : public Color {
		using Color::Color;

		BorderColor(const Color& c) : Color(c) {}
	};

	struct TempInteraction {
		flecs::entity entity;
		float stack_index;
		FocusStrategy focus_strategy;
	};

	struct UiModule {
		UiModule(flecs::world& world);
	};
}
