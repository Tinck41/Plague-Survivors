#include "layout.h"

#include <algorithm>

using namespace ps;

void LayoutComposer::push_node(LayoutNode node) {
	if (!node.fixed.first) {
		node.size.x = node.min_size.first.value_or(node.size.x);
	}
	if (!node.fixed.second) {
		node.size.y = node.min_size.second.value_or(node.size.y);
	}

	if (node.stack_index == 1) {
		nodes.push_back(node);

		return;
	}

	const auto last_node = nodes.back();

	nodes.push_back(node);

	if (last_node.parent == 0 || last_node.parent_stack_index != node.parent_stack_index) {
		std::sort(nodes.begin() + nodes[last_node.parent].first_child, nodes.begin() + nodes[last_node.parent].last_child + 1, [](const LayoutNode& lhs, const LayoutNode& rhs) {
			return lhs.stack_index < rhs.stack_index;
		});

		auto parent_it = std::ranges::find(nodes.begin() + nodes[last_node.parent].first_child, nodes.begin() + nodes[last_node.parent].last_child + 1, node.parent_stack_index, &LayoutNode::stack_index);

		nodes.back().parent = std::distance(nodes.begin(), parent_it);

		parent_it->first_child = nodes.size() - 1;
		parent_it->last_child = nodes.size() - 1;
	}
	else {
		nodes.back().parent = last_node.parent == 0 ? nodes.size() - 2 : last_node.parent;

		nodes[nodes.back().parent].last_child = nodes.size() - 1;
	}
}

void LayoutComposer::build() {
	const auto& last_parent = nodes[nodes.back().parent];

	if ((last_parent.last_child - last_parent.first_child) >= 1 && last_parent.stack_index != 0) {
		std::sort(
			nodes.begin() + last_parent.first_child,
			nodes.begin() + last_parent.last_child + 1,
			[](const LayoutNode& lhs, const LayoutNode& rhs) {
				return lhs.stack_index < rhs.stack_index;
			}
		);
	}

	calculate_fit_width();
	calculate_grow_shrink_width();
	wrap_text();
	calculate_fit_height();
	calculate_grow_shrink_height();
	calculate_positions();

	std::ranges::sort(nodes, {}, &LayoutNode::stack_index);
}

void LayoutComposer::clear() {
	nodes.erase(nodes.begin() + 1, nodes.end());
}

void LayoutComposer::calculate_fit_width() {
	reverse_bfs(nodes, [&](LayoutNode& node) {
		auto& parent = nodes[node.parent];

		if (node.fit.first) {
			node.size.x += node.padding.x + node.padding.y;

			if (node.horizontal) {
				const auto child_gap = node.child_gap * static_cast<float>(node.last_child - node.first_child);

				node.size.x += child_gap.x;
			}
		}

		if (parent.fit.first) {
			if (parent.horizontal) {
				parent.size.x += node.size.x;
			}
			else {
				parent.size.x = std::max(node.size.x, parent.size.x);
			}
		}
	});
}

void LayoutComposer::calculate_fit_height() {
	reverse_bfs(nodes, [&](LayoutNode& node) {
		auto& parent = nodes[node.parent];

		if (node.fit.second) {
			node.size.y += node.padding.z + node.padding.w;

			if (!node.horizontal) {
				const auto child_gap = node.child_gap * static_cast<float>(node.last_child - node.first_child);

				node.size.y += child_gap.y;
			}
		}

		if (parent.fit.second) {
			if (parent.horizontal) {
				parent.size.y = std::max(node.size.y, parent.size.y);
			}
			else {
				parent.size.y += node.size.y;
			}
		}
	});
}

void LayoutComposer::calculate_grow_shrink_width() {
	bfs(nodes, [&](LayoutNode& node) {
		if (node.fit.first) {
			return;
		}

		std::vector<size_t> growable;
		std::vector<size_t> shrinkable;

		growable.reserve(node.last_child - node.first_child + 1);
		shrinkable.reserve(node.last_child - node.first_child + 1);

		auto remaining_size = node.size.x;

		remaining_size -= node.padding.x + node.padding.y;
		if (node.horizontal) {
			remaining_size -= node.child_gap.x * static_cast<float>(node.last_child - node.first_child);
		}

		for (size_t i = node.first_child; i <= node.last_child; ++i) {
			if (node.horizontal) {
				remaining_size -= nodes[i].size.x;
			}

			if (nodes[i].grow.first) {
				growable.push_back(i);
				shrinkable.push_back(i);

				if (!node.horizontal) {
					nodes[i].size.x += node.size.x - node.padding.x - node.padding.y - nodes[i].size.x;
				}
			}
		}

		node.remaining_size.x = remaining_size;

		if (!node.horizontal) {
			return;
		}

		growable.shrink_to_fit();
		shrinkable.shrink_to_fit();

		while (!growable.empty() && remaining_size > 0.f) {
			auto smallest = std::numeric_limits<float>::max();
			auto second_smallest = std::numeric_limits<float>::max();
			auto size_to_add = remaining_size;

			for (auto i : growable) {
				if (nodes[i].size.x < smallest) {
					second_smallest = smallest;
					smallest = nodes[i].size.x;
				}

				if (nodes[i].size.x > smallest) {
					second_smallest = std::min(second_smallest, nodes[i].size.x);
					size_to_add = second_smallest - smallest;
				}
			}

			size_to_add = std::min(size_to_add, remaining_size / growable.size());

			for (auto it = growable.begin(); it != growable.end();) {
				float previous_size = nodes[*it].size.x;
				bool erase = false;

				if (nodes[*it].size.x == smallest) {
					nodes[*it].size.x += size_to_add;

					if (nodes[*it].max_size.first && nodes[*it].size.x >= nodes[*it].max_size.first.value()) {
						nodes[*it].size.x = nodes[*it].max_size.first.value();

						erase = true;
					}

					remaining_size -= (nodes[*it].size.x - previous_size);
				}

				if (erase) {
					it = growable.erase(it);
				}
				else {
					++it;
				}
			}
		}

		while (!shrinkable.empty() && remaining_size < 0.f) {
			auto largest = 0.f;
			auto second_largest = 0.f;
			auto size_to_sub = remaining_size;

			for (auto i : shrinkable) {
				if (nodes[i].size.x > largest) {
					second_largest = largest;
					largest = nodes[i].size.x;
				}

				if (nodes[i].size.x < largest) {
					second_largest = std::max(second_largest, nodes[i].size.x);
					size_to_sub = second_largest - largest;
				}
			}

			size_to_sub = std::max(size_to_sub, remaining_size / shrinkable.size());

			for (auto it = shrinkable.begin(); it != shrinkable.end();) {
				float previous_size = nodes[*it].size.x;
				bool erase = false;

				if (nodes[*it].size.x == largest) {
					nodes[*it].size.x += size_to_sub;

					if (nodes[*it].min_size.first && nodes[*it].size.x <= nodes[*it].min_size.first.value()) {
						nodes[*it].size.x = nodes[*it].min_size.first.value();

						erase = true;
					}

					remaining_size -= (nodes[*it].size.x - previous_size);
				}

				if (erase) {
					it = shrinkable.erase(it);
				}
				else {
					++it;
				}
			}
		}

		node.remaining_size.x = remaining_size;
	});
}

void LayoutComposer::calculate_grow_shrink_height() {
	bfs(nodes, [&](LayoutNode& node) {
		if (node.fit.second) {
			return;
		}

		std::vector<size_t> growable;
		std::vector<size_t> shrinkable;

		growable.reserve(node.last_child - node.first_child);
		shrinkable.reserve(node.last_child - node.first_child);

		auto remaining_size = node.size.y;

		remaining_size -= node.padding.z + node.padding.w;
		if (!node.horizontal) {
			remaining_size -= node.child_gap.y * static_cast<float>(node.last_child - node.first_child);
		}

		for (size_t i = node.first_child; i <= node.last_child; ++i) {
			if (!node.horizontal) {
				remaining_size -= nodes[i].size.y;
			}

			if (nodes[i].grow.second) {
				growable.push_back(i);
				shrinkable.push_back(i);

				if (node.horizontal) {
					nodes[i].size.y += node.size.y - node.padding.z - node.padding.w - nodes[i].size.y;
				}
			}
		}

		node.remaining_size.y = remaining_size;

		if (node.horizontal) {
			return;
		}

		growable.shrink_to_fit();
		shrinkable.shrink_to_fit();

		while (!growable.empty() && remaining_size > 0.f) {
			auto smallest = std::numeric_limits<float>::max();
			auto second_smallest = std::numeric_limits<float>::max();
			auto size_to_add = remaining_size;

			for (auto i : growable) {
				if (nodes[i].size.y < smallest) {
					second_smallest = smallest;
					smallest = nodes[i].size.y;
				}

				if (nodes[i].size.y > smallest) {
					second_smallest = std::min(second_smallest, nodes[i].size.y);
					size_to_add = second_smallest - smallest;
				}
			}

			size_to_add = std::min(size_to_add, remaining_size / growable.size());

			for (auto it = growable.begin(); it != growable.end();) {
				float previous_size = nodes[*it].size.y;
				bool erase = false;

				if (nodes[*it].size.y == smallest) {
					nodes[*it].size.y += size_to_add;

					if (nodes[*it].max_size.second && nodes[*it].size.y >= nodes[*it].max_size.second.value()) {
						nodes[*it].size.y = nodes[*it].max_size.second.value();

						erase = true;
					}

					remaining_size -= (nodes[*it].size.y - previous_size);
				}

				if (erase) {
					it = growable.erase(it);
				}
				else {
					++it;
				}
			}
		}

		while (!shrinkable.empty() && remaining_size < 0.f) {
			auto largest = 0.f;
			auto second_largest = 0.f;
			auto size_to_sub = remaining_size;

			for (auto i : shrinkable) {
				if (nodes[i].size.y > largest) {
					second_largest = largest;
					largest = nodes[i].size.y;
				}

				if (nodes[i].size.y < largest) {
					second_largest = std::max(second_largest, nodes[i].size.y);
					size_to_sub = second_largest - largest;
				}
			}

			size_to_sub = std::max(size_to_sub, remaining_size / shrinkable.size());

			for (auto it = shrinkable.begin(); it != shrinkable.end();) {
				float previous_size = nodes[*it].size.y;
				bool erase = false;

				if (nodes[*it].size.y == largest) {
					nodes[*it].size.y += remaining_size;

					if (nodes[*it].min_size.second && nodes[*it].size.y <= nodes[*it].min_size.second.value()) {
						nodes[*it].size.y = nodes[*it].min_size.second.value();

						erase = true;
					}

					remaining_size -= nodes[*it].size.y - previous_size;
				}

				if (erase) {
					it = shrinkable.erase(it);
				}
				else {
					++it;
				}
			}
		}

		node.remaining_size.y = remaining_size;
	});

}

void LayoutComposer::wrap_text() {
	bfs(nodes, [&](LayoutNode& node) {
		if (!node.text_data) {
			return;
		}

		auto& text_data = node.text_data.value();

		std::string result;

		size_t last_word_index = 0;
		size_t first_word_index = 0;

		for (size_t i = 0; i < text_data.text.size(); ++i) {
			if (text_data.text[i] != ' ' && text_data.text[i] != '\n' && i != text_data.text.size() - 1) {
				continue;
			}

			glm::ivec2 line_size;
			const auto length = i - first_word_index;

			TTF_GetStringSize(text_data.font, text_data.text.substr(first_word_index, length).c_str(), length, &line_size.x, &line_size.y);

			line_size = glm::vec2(line_size) * text_data.font_scale;

			if (line_size.x > node.size.x) {
				size_t offset = 0;

				while (text_data.text[last_word_index + offset] == ' ') {
					++offset;
				}

				result.append(text_data.text, first_word_index, last_word_index - first_word_index + offset);
				result.push_back('\n');

				first_word_index = last_word_index + offset;

				if (i == text_data.text.size() - 1) {
					result.append(text_data.text, first_word_index, length + 1);
				}
			}
			else if (i == text_data.text.size() - 1) {
					result.append(text_data.text, first_word_index, length + 1);
			}

			last_word_index = i;
		}

		text_data.text = result;
	});
}

void LayoutComposer::calculate_positions() {
	bfs(nodes, [&](LayoutNode& node) {
		auto& parent = nodes[node.parent];

		node.offsets.x += node.padding.x;
		node.offsets.y += node.padding.z;

		if (node.horizontal) {
			node.offsets.x += node.remaining_size.x * node.child_alignment.x;
		}
		else {
			node.offsets.y += node.remaining_size.y * node.child_alignment.y;
		}

		if (node.parent == 0) {
			return;
		}

		node.pos.x += parent.offsets.x;
		node.pos.y += parent.offsets.y;

		if (parent.horizontal) {
			const auto available_height = parent.size.y - parent.padding.z - parent.padding.w - node.size.y;

			parent.offsets.x += node.size.x + parent.child_gap.x;

			if (node.self_alignment.second) {
				node.pos.y += (available_height) * node.self_alignment.second.value();
			}
			else {
				node.pos.y += (available_height) * parent.child_alignment.y;
			}
		}
		else {
			const auto available_width = parent.size.x - parent.padding.x - parent.padding.y - node.size.x;

			parent.offsets.y += node.size.y + parent.child_gap.y;

			if (node.self_alignment.first) {
				node.pos.x += (available_width) * node.self_alignment.first.value();
			}
			else {
				node.pos.x += (available_width) * parent.child_alignment.x;
			}
		}
	});
}

void LayoutComposer::bfs(std::vector<LayoutNode>& nodes, std::function<void(LayoutNode&)> callback) {
	for (size_t i = 1; i < nodes.size(); ++i) {
		callback(nodes[i]);
	}
}

void LayoutComposer::reverse_bfs(std::vector<LayoutNode>& nodes, std::function<void(LayoutNode&)> callback) {
	for (size_t i = nodes.size() - 1; i > 0; --i) {
		callback(nodes[i]);
	}
}

size_t LayoutComposer::get_children_count(size_t parent) {
	return nodes[parent].last_child - nodes[parent].first_child + 1;
}

bool LayoutComposer::has_children(size_t parent) {
	return nodes[parent].first_child != 0;
}

