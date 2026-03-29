#include "module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/utils.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/textModule/module.h"

#include <vector>

using namespace ps;

UiModule::UiModule(flecs::world& world) {
	world.module<UiModule>();

	world.import<TransformModule>();
	world.import<InputModule>();
	world.import<TextModule>();
	world.import<CameraModule>();

	world.component<NodeVector>()
		.add(flecs::Singleton);

	world.component<UiTreeChanged>()
		.add(flecs::Singleton);

	world.component<CustomNodeIndex>()
		.member<size_t>("index");

	world.component<BackgroundColor>()
		.is_a<Color>();

	world.component<Node>()
		.member<glm::vec2>("size")
		.member<size_t>("stack_index")
		.add(flecs::With, world.component<RenderLayers>())
		.add(flecs::With, world.component<Aabb>())
		.add(flecs::With, world.component<Visible2d>())
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<BackgroundColor>())
		.add(flecs::With, flecs::OrderedChildren);

	world.component<Image>()
		.add(flecs::With, world.component<Node>());

	world.component<Text>()
		.is_a<std::string>()
		.add(flecs::With, world.component<TextData>())
		.add(flecs::With, world.component<TextFont>())
		.add(flecs::With, world.component<TextColor>())
		.add(flecs::With, world.component<Node>());

	world.component<Interaction>()
		.constant("None", Interaction::None)
		.constant("Hovered", Interaction::Hovered)
		.constant("Clicked", Interaction::Clicked)
		.add(flecs::Exclusive);

	world.component<FocusStrategy>()
		.constant("Block", FocusStrategy::Block)
		.constant("Pass", FocusStrategy::Pass)
		.add(flecs::Exclusive);

	world.component<Button>()
		.add(flecs::With, world.component<Interaction>())
		.add(flecs::With, world.component<FocusStrategy>())
		.add(flecs::With, world.component<Node>());

	world.observer<CustomNodeIndex>()
		.event(flecs::OnSet)
		.with(flecs::ChildOf, flecs::Wildcard)
		.with<Node>()
		.each([&world](flecs::entity child, CustomNodeIndex& customIndex) {
			auto parent = child.parent();
			auto children = utils::get_children(parent);

			std::erase(children, child);
			children.insert(children.begin() + customIndex.value, child);
			parent.set_child_order(children.data(), children.size());

			child.remove<CustomNodeIndex>(); // TODO: is it right?
			world.add<UiTreeChanged>();
		});

	world.observer<Node>()
		.event(flecs::OnAdd)
		.with(flecs::ChildOf, flecs::Wildcard)
		.each([&world](flecs::entity child, Node& node) {
			world.add<UiTreeChanged>();
		});

	world.observer<RenderLayers, Node>()
		.term_at(1).filter()
		.event(flecs::OnAdd)
		.each([&world](RenderLayers& render_layers, Node& node) {
			render_layers.mask = RenderLayers::UI;
		});

	world.system<NodeVector>()
		.with<UiTreeChanged>()
		.with<Node>()
		.without(flecs::ChildOf, flecs::Wildcard) // TODO: SceneRoot in future
		.kind(Phases::PreUpdate)
		.each([&world](flecs::entity e, NodeVector& node_vector) {
			auto index = -1;

			node_vector.sorted_nodes.clear();

			utils::dfs(e, [&index, &node_vector](flecs::entity child) {
				child.get_ref<Node>()->stack_index = ++index;
				node_vector.sorted_nodes.emplace_back(child);
			});

			world.remove<UiTreeChanged>();
		});

	world.system<const Node, Interaction, const FocusStrategy, const GlobalTransform>("focus interaction")
		.kind(Phases::Update)
		.order_by<Node>([](flecs::entity_t e1, const Node* n1, flecs::entity_t e2, const Node* n2) {
			return (n1->stack_index < n2->stack_index) - (n1->stack_index > n2->stack_index);
		})
		.run([&world](flecs::iter& it) {
			const auto& input = world.get<Input>();

			auto last_focus_strategy = FocusStrategy::Pass;

			while(it.next()) {
				const auto nodes = it.field<const Node>(0);
				const auto interactions = it.field<Interaction>(1);
				const auto foucs_strategies = it.field<const FocusStrategy>(2);
				const auto transforms = it.field<const GlobalTransform>(3);

				for (auto i : it) {
					const auto& node = nodes[i];
					auto& interaction = interactions[i];
					const auto& foucs_strategie = foucs_strategies[i];
					const auto& transform = transforms[i];

					interaction = Interaction::None;

					if (last_focus_strategy == FocusStrategy::Block) {
						continue;
					}

					const auto& mouse_pos = input.mouse.position;
					const auto& pos = transform.translation;
					const auto& size = node.size;

					if (mouse_pos.x >= pos.x && mouse_pos.x <= (pos.x + size.x) && mouse_pos.y >= pos.y && mouse_pos.y <= (pos.y + size.y)) {
						if (input.mouse.left.pressed || input.mouse.left.remain) {
							interaction = Interaction::Clicked;
						}
						else {
							interaction = Interaction::Hovered;
						}

						last_focus_strategy = foucs_strategie;
					}
				}
			}
		});

	world.add<NodeVector>();
}
