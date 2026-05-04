#pragma once

#include "flecs.h"

namespace ps {
	flecs::entity create_tree_node(flecs::world& world, flecs::entity source);
	flecs::entity create_transform_info(flecs::world& world, flecs::entity source);
}
