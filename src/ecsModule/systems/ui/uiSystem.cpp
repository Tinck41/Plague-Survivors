#include "uiSystem.h"

#include "SFML/Graphics/Color.hpp"
#include "ecsModule/components/rendererComponent.h"
#include "ecsModule/components/transformComponent.h"
#include "ecsModule/components/ui/imageComponent.h"
#include "ecsModule/components/ui/nodeBundle.h"
#include "ecsModule/components/ui/buttonBundle.h"
#include "ecsModule/components/ui/nodeComponent.h"
#include "ecsModule/components/ui/styleComponent.h"
#include "ecsModule/ecsController.h"
#include "ecsModule/types.h"

using namespace ps;
using namespace ecsModule;
using namespace ui;

void UiSystem::create() {
	auto& world = EcsControllerInstance::getInstance()->getWorld();

	// register bundles
	world.prefab<NodeBundle>()
		.add<NodeComponent>()
		.add<StyleComponent>()
		.add<TransformComponent>();

	world.prefab<ButtonBundle>()
		.add<NodeComponent>()
		.add<StyleComponent>()
		.add<ImageComponent>()
		.add<TransformComponent>();
	
	// init systems
	world.system<StyleComponent>()
		.kind(flecs::OnStore)
		.each([](StyleComponent& style) {
			style.shape.setSize(style.size);
			style.shape.setFillColor(style.backgroundColor);
		});

	// propably call draw twice on same node with two NodeComponent
	world.system<RendererComponent, NodeComponent>()
		.term_at(0).singleton()
		.kind(Phases::Render)
		.each([](flecs::iter& it, size_t size, RendererComponent& r, NodeComponent) {
			auto entity = it.entity(size);
			drawCall(entity, r);
		});

	// create ui entities
    const auto windowSize = [&]() {
        const auto windowSize = world.get<RendererComponent>()->renderer->getSize();
        return sf::Vector2f { static_cast<float>(windowSize.x), static_cast<float>(windowSize.y) };
    }();
	const auto canvas = world.entity()
		.is_a<NodeBundle>()
		.set(StyleComponent {
			.size = windowSize,
            .backgroundColor = sf::Color::Blue,
            .shape = sf::RectangleShape{ windowSize }
		});

    auto button = world.entity()
        .child_of(canvas)
        .is_a<ButtonBundle>()
        .set(StyleComponent {
            .size = { 60.f, 600.f },
            .backgroundColor = sf::Color::Cyan,
            .shape = sf::RectangleShape{}
        })
        .set(TransformComponent {
            .translation = {-120.f, 0.f }
        });
}

void UiSystem::drawCall(flecs::entity entity, RendererComponent& renderer) {
	if (entity.has<StyleComponent>()) {
		const auto& style = entity.get<StyleComponent>();
		renderer.renderer->draw(style->shape);
	}
	entity.children([&](flecs::entity entity) {
		drawCall(entity, renderer);
	});
}
