#include "functions.h"

#include "ecsModule/editor_module/components.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "glaze/glaze.hpp"

#include <format>

flecs::entity ps::create_tree_node(flecs::world& world, flecs::entity source) {
	auto container = world.entity(std::format("tree_node_#{}", source.name().size() > 0 ? source.name().c_str() : std::to_string(source.id()).c_str()).c_str())
		.add<Button>()
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Fit{} },
			.grow_direction = Node::GrowDirection::Vertical,
		});

	auto self_container = world.entity(std::format("self_container_#{}", container.id()).c_str())
		.child_of(container)
		.add<TreeNodePart>()
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Fit{} },
			.child_alignment = { 0.f, 0.5f }
		});

	auto button = world.entity(std::format("button_#{}", container.id()).c_str())
		.child_of(self_container)
		.add<TreeNodePart>()
		.add<Button>()
		.set<Node>({
			.sizing_policy = { Node::Fixed{ 25.f }, Node::Fixed{ 25.f } },
		})
		.set<BackgroundColor>(Color::from_hex("#79740e"));

	auto name = world.entity(std::format("name_#{}", container.id()).c_str())
		.child_of(self_container)
		.add<TreeNodePart>()
		.set<Node>({
			.sizing_policy = { Node::Fixed{ 250.f }, Node::Grow{} },
		})
		.set<Text>({ source.name().size() > 0 ? source.name().c_str() : std::to_string(source.id()) })
		.set<TextFont>({
			.handle = world.get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
			.size = 24,
		})
		.set<BackgroundColor>(TRANSPARENT)
		.set<TextColor>(WHITE);

	auto nodes_container = world.entity(std::format("nodes_container_#{}", container.id()).c_str())
		.child_of(container)
		.add<TreeNodePart>()
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Fit{} },
			.child_gap = { 0.f, 5.f },
			.padding = { 25.f, 0.f, 0.f, 0.f },
			.grow_direction = Node::GrowDirection::Vertical,
		})
		.set<BackgroundColor>(TRANSPARENT);

	container.set<TreeNode>({ 
		.entity = source,
		.container = nodes_container,
		.self_container = self_container,
	});

	return container;
}

flecs::entity ps::create_transform_info(flecs::world& world, flecs::entity source) {
	return world.entity();
}
