#pragma once

#include "glm.hpp"
#include "SDL3_ttf/SDL_ttf.h"

#include <functional>
#include <vector>
#include <optional>

namespace ps {
	struct LayoutNode {
		struct TextData {
			std::string text;
			TTF_Font* font;
			float font_scale;
		};

		size_t parent = 0;
		size_t first_child = 0;
		size_t last_child = 0;

		size_t parent_stack_index = 0;
		size_t stack_index = 0;

		bool horizontal;

		glm::vec2 size;
		glm::vec2 child_alignment;
		glm::vec2 pos;
		glm::vec2 child_gap;
		glm::vec4 padding;

		glm::vec2 offsets;
		glm::vec2 remaining_size;

		std::pair<bool, bool> fixed;
		std::pair<bool, bool> grow;
		std::pair<bool, bool> fit;

		std::pair<std::optional<float>, std::optional<float>> self_alignment;
		std::pair<std::optional<float>, std::optional<float>> min_size;
		std::pair<std::optional<float>, std::optional<float>> max_size;

		std::optional<TextData> text_data;
	};

	class LayoutComposer {
	public:
		void push_node(LayoutNode node);
		void build();
		void clear();

		const LayoutNode& operator[](size_t index) const { return nodes[index]; }
	private:
		void calculate_fit_width();
		void calculate_fit_height();
		void calculate_grow_shrink_width();
		void calculate_grow_shrink_height();
		void wrap_text();
		void calculate_positions();

		void bfs(std::vector<LayoutNode>& nodes, std::function<void(LayoutNode&)> callback);
		void reverse_bfs(std::vector<LayoutNode>& nodes, std::function<void(LayoutNode&)> callback);

		size_t get_children_count(size_t parent);

		bool has_children(size_t parent);

		std::vector<LayoutNode> nodes{ LayoutNode{} };
	};
}
