#pragma once

#include "flecs.h"

#include <functional>

namespace ps::utils {
	void dfs(flecs::entity e, std::function<void(flecs::entity)> callback);
}
