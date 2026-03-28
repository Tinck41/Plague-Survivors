#pragma once

#include "SDL3/SDL.h"
#include "flecs.h"
#include "texture.h"
#include "glm.hpp"
#include "color.h"

#include <memory>
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
		Color color;
	};

	struct Text : public std::string {
		using std::string::string;

		Text(const std::string& string) : std::string(string) {}
	};

	struct BackgroundColor : public Color {
		using Color::Color;

		BackgroundColor(const Color& c) : Color(c) {}
	};

	struct UiModule {
		UiModule(flecs::world& world);
	};
}
