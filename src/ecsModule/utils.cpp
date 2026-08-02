#include "utils.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <queue>

void ps::utils::dfs(flecs::entity e, std::function<void(flecs::entity)> callback) {
	callback(e);

	e.children([callback](flecs::entity child) {
		dfs(child, callback);
	});
}

void ps::utils::dfs(flecs::entity e, size_t depth, std::function<void(flecs::entity, const size_t)> callback) {
	callback(e, depth);

	e.children([callback, depth = depth + 1](flecs::entity child) {
		dfs(child, depth, callback);
	});
}

void ps::utils::dfs(std::span<flecs::entity> roots, std::function<void(flecs::entity)> callback) {
	for (const auto& root : roots) {
		dfs(root, callback);
	}
}

void ps::utils::bfs(flecs::entity e, std::function<void(flecs::entity)> callback) {
	std::queue<flecs::entity> queue;
	queue.push(e);

	while (!queue.empty()) {
		auto entity = queue.front();
		queue.pop();

		callback(entity);

		entity.children([&queue](flecs::entity child) {
			queue.push(child);
		});
	}
}

void ps::utils::bfs(std::span<flecs::entity> roots, std::function<void(flecs::entity)> callback) {
	std::queue<flecs::entity> queue;
	for (const auto& root : roots) {
		queue.push(root);
	}

	while (!queue.empty()) {
		auto entity = queue.front();
		queue.pop();

		callback(entity);

		entity.children([&queue](flecs::entity child) {
			queue.push(child);
		});
	}
}

void ps::utils::insert_child(flecs::entity parent, flecs::entity child, size_t position) {
	child.child_of(parent);

	if (position != -1 && parent.has(flecs::OrderedChildren)) {
		auto children = get_children(parent);
		children.insert(children.begin() + position, child);
		parent.set_child_order(children.data(), children.size());
	}
}

void ps::utils::insert_child_back(flecs::entity parent, flecs::entity child) {
	child.child_of(parent);

	if (parent.has(flecs::OrderedChildren)) {
		auto children = get_children(parent);

		std::erase(children, child);

		children.push_back(child);
		parent.set_child_order(children.data(), children.size());
	}
}

void ps::utils::insert_child_before(flecs::entity parent, flecs::entity child, flecs::entity before) {
	child.child_of(parent);

	if (parent.has(flecs::OrderedChildren)) {
		auto children = get_children(parent);

		auto it = std::ranges::find(children, before);

		if (it != children.end()) {
			auto [first, last] = std::ranges::remove_if(children, [id = child.id()](flecs::entity_t child) {
				return child == id;
			});

			children.erase(first, last);
			children.insert(it, child);
		}

		parent.set_child_order(children.data(), children.size());
	}
}

size_t ps::utils::get_children_count(const flecs::entity& parent) {
	return ecs_get_ordered_children(parent.world(), parent).count;
}

std::vector<flecs::entity_t> ps::utils::get_children(const flecs::entity& parent) {
	std::vector<flecs::entity_t> children;

	parent.children([&children](flecs::entity child) {
		children.emplace_back(child);
	});

	return children;
}

std::vector<flecs::entity_t> ps::utils::get_alive_children(const flecs::entity& parent) {
	std::vector<flecs::entity_t> children;

	parent.children([&children](flecs::entity child) {
		if (!child.is_alive()) {
			return;
		}

		children.emplace_back(child);
	});

	return children;
}

std::vector<flecs::entity_t> ps::utils::get_all_children_dfs(const flecs::entity& root) {
	std::vector<flecs::entity_t> children;

	dfs(root, [&children](flecs::entity e) {
		children.emplace_back(e);
	});

	return children;
}
