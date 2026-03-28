#include "utils.h"

void ps::utils::dfs(flecs::entity e, std::function<void(flecs::entity)> callback) {
	callback(e);

	e.children([callback](flecs::entity child) {
		dfs(child, callback);
	});
}

void ps::utils::insert_child(flecs::entity parent, flecs::entity child, size_t position) {
	child.child_of(parent);

	if (position != -1 && parent.has(flecs::OrderedChildren)) {
		auto children = get_children(parent);
		children.insert(children.begin() + position, child);
		parent.set_child_order(children.data(), children.size());
	}
}

std::vector<flecs::entity_t> ps::utils::get_children(const flecs::entity& parent) {
	std::vector<flecs::entity_t> children;

	parent.children([&children](flecs::entity child) {
		children.emplace_back(child);
	});

	return children;
}
