#include "utils.h"

void ps::utils::dfs(flecs::entity e, std::function<void(flecs::entity)> callback) {
	callback(e);

	e.children([callback](flecs::entity child) {
		dfs(child, callback);
	});
}

