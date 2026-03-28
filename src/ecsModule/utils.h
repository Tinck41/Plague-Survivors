#pragma once

#include "flecs.h"

#include <functional>

namespace ps::utils {
	void dfs(flecs::entity e, std::function<void(flecs::entity)> callback);
	void insert_child(flecs::entity parent, flecs::entity child, size_t position = -1);

	std::vector<flecs::entity_t> get_children(const flecs::entity& parent);
}
