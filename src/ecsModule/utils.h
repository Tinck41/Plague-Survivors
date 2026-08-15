#pragma once

#include "flecs.h"

#include <functional>
#include <span>

namespace se::utils {
	void dfs(flecs::entity e, std::function<void(flecs::entity)> callback);
	void dfs(flecs::entity e, size_t depth, std::function<void(flecs::entity, const size_t)> callback);
	void dfs(std::span<flecs::entity> roots, std::function<void(flecs::entity)> callback);
	void bfs(flecs::entity e, std::function<void(flecs::entity)> callback);
	void bfs(std::span<flecs::entity> roots, std::function<void(flecs::entity)> callback);
	void insert_child(flecs::entity parent, flecs::entity child, size_t position = -1);
	void insert_child_back(flecs::entity parent, flecs::entity child);
	void insert_child_before(flecs::entity parent, flecs::entity child, flecs::entity before);

	size_t get_children_count(const flecs::entity& parent);

	std::vector<flecs::entity_t> get_children(const flecs::entity& parent);
	std::vector<flecs::entity_t> get_alive_children(const flecs::entity& parent);
	std::vector<flecs::entity_t> get_all_children_dfs(const flecs::entity& root);
}
