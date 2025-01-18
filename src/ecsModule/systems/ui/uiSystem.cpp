#include "uiSystem.h"

#include "SFML/Graphics/Color.hpp"
#include "ecsModule/components/rendererComponent.h"
#include "ecsModule/components/transformComponent.h"
#include "ecsModule/components/inputComponent.h"
#include "ecsModule/components/ui/imageComponent.h"
#include "ecsModule/components/ui/nodeBundle.h"
#include "ecsModule/components/ui/buttonBundle.h"
#include "ecsModule/components/ui/nodeComponent.h"
#include "ecsModule/components/ui/styleComponent.h"
#include "ecsModule/components/ui/interactionComponent.h"
#include "ecsModule/ecsController.h"
#include "ecsModule/types.h"
#include "spdlog/spdlog.h"

using namespace ps;
using namespace ecsModule;
using namespace ui;

void UiSystem::create() {
	auto& world = EcsControllerInstance::getInstance()->getWorld();

	// register bundles
	world.prefab<NodeBundle>()
		.add<NodeComponent>()
		.add<StyleComponent>()
        .add<InteractionComponent>()
		.add<TransformComponent>();

	world.prefab<ButtonBundle>()
		.add<NodeComponent>()
		.add<StyleComponent>()
		.add<ImageComponent>()
        .add<InteractionComponent>()
		.add<TransformComponent>();
	
	// init systems
    world.observer<TransformComponent>()
        .event(flecs::OnSet)
        .each([](flecs::entity e, TransformComponent& t) {
            auto style = e.get_mut<StyleComponent>();
            if (style) {
                style->shape.setPosition(t.translation);
            }
        });
	world.observer<StyleComponent>()
		.event(flecs::OnSet)
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

    world.system<InteractionComponent*, InteractionComponent, StyleComponent>()
            .term_at(0).inout(flecs::InOutNone).cascade(flecs::ChildOf).desc().oper(flecs::Optional)
            .kind(Phases::Update)
            .run([](flecs::iter& it) {
                auto world = it.world();
                auto renderer = world.get<RendererComponent>();
                auto input = world.get<InputComponent>();
                auto hovered = false;

                if (!renderer->renderer->hasFocus()) {
                    it.fini();
                    return;
                }

                while (it.next()) {
                    auto interaction = it.field<InteractionComponent>(1);
                    auto style = it.field<StyleComponent>(2);


                    for (auto i : it) {
                        spdlog::info(it.entity(i).path());
                        const auto mouseInsideRect = [&] {
                            const auto entityShape = style[i].shape;
                            const auto mousePos = input->mouse.position;

                            const auto globalBounds = entityShape.getGlobalBounds();

                            const auto containsClient = globalBounds.contains(mousePos);

                            return containsClient;
                        }();

                        if (mouseInsideRect) {
                            //spdlog::info("mouser inside {}, hovered: {}", entity.name(), hovered);
                            if (!hovered && interaction[i].type != InteractionComponent::Type::Hovered) {
                                interaction[i].type = InteractionComponent::Type::Hovered;
                                hovered = true;
                            } else {
                                if (hovered) {
                                    interaction[i].type = InteractionComponent::Type::None;
                                } else {
                                    hovered = true;
                                }
                            }
                        } else {
                            interaction[i].type = InteractionComponent::Type::None;
                        }
                    }
                }
            });

    world.system<InteractionComponent, StyleComponent>()
            .kind(Phases::Update)
            .each([](flecs::iter& it, size_t size, InteractionComponent& i, StyleComponent& s) {
                auto entity = it.entity(size);
                const auto type = i.type;
                const auto oldColor = s.backgroundColor;
                switch (type) {
                    case InteractionComponent::Type::Hovered:
                        spdlog::info("{} is hovered", entity.name());
                        s.backgroundColor = sf::Color::Red;
                        break;
                    case InteractionComponent::Type::Clicked:
                        s.backgroundColor = sf::Color::Green;
                        break;
                    case InteractionComponent::Type::None:
                        s.backgroundColor = sf::Color::Blue;
                    default:
                        break;
                }
                if (oldColor != s.backgroundColor) {
                    entity.modified<StyleComponent>();
                }
            });

	// create ui entities
    const auto windowSize = [&]() {
        const auto windowSize = world.get<RendererComponent>()->renderer->getSize();
        return sf::Vector2f { static_cast<float>(windowSize.x), static_cast<float>(windowSize.y) };
    }();
	const auto canvas = world.entity("canvas")
		.is_a<NodeBundle>()
		.set(StyleComponent {
			.size = windowSize,
            .backgroundColor = sf::Color::Blue,
            .shape = sf::RectangleShape{ windowSize }
		});

    auto button = world.entity("button")
        .child_of(canvas)
        .is_a<ButtonBundle>()
        .set(StyleComponent {
            .size = { 200.f, 60.f },
            .backgroundColor = sf::Color::Cyan,
            .shape = sf::RectangleShape{}
        })
        .set(TransformComponent {
            .translation = {10.f, 0.f }
        });

    auto button2 = world.entity("button2")
            .child_of(canvas)
            .is_a<ButtonBundle>()
            .set(StyleComponent {
                .size = { 200.f, 60.f },
                .backgroundColor = sf::Color::Cyan,
                .shape = sf::RectangleShape{}
            })
            .set(TransformComponent {
                .translation = {10.f, 80.f }
            });
    auto button3 = world.entity("button3")
        .child_of(button2)
        .is_a<ButtonBundle>()
        .set(StyleComponent {
            .size = { 50.f, 50.f },
            .backgroundColor = sf::Color::Cyan,
            .shape = sf::RectangleShape{}
        })
        .set(TransformComponent {
            .translation = { 210.f, 80.f }
        });
	auto button4 = world.entity("button4")
		.child_of(button2)
		.is_a<ButtonBundle>()
		.set(StyleComponent {
			.size = { 100.f, 100.f },
			.backgroundColor = sf::Color::Cyan,
            .shape = sf::RectangleShape{}
        })
        .set(TransformComponent {
            .translation = { 210.f, 80.f }
		});
    auto button5 = world.entity("button5")
            .child_of(button4)
            .is_a<ButtonBundle>();
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

bool UiSystem::mouseEvent(flecs::entity entity, RendererComponent &renderer, InteractionComponent &interaction, bool hovered) {
    if (!renderer.renderer->hasFocus()) {
        return false;
    }

    auto& world = EcsControllerInstance::getInstance()->getWorld();
    auto input = world.get<InputComponent>();

    const auto moved = input->mouse.moved;
    const auto pressed = input->mouse.left.pressed;
    const auto released = input->mouse.left.released;
    //if (!moved && !pressed && !released) {
    //    interaction.type = InteractionComponent::Type::None;
    //    return false;
    //}

    //entity.children([&](flecs::entity entity) {
    //    if (mouseEvent(entity, renderer, interaction)) {
    //       return;
    //    }
    //});

    const auto mouseInsideRect = [&] {
        const auto entityPos = entity.get<TransformComponent>()->translation;
        const auto entitySize = entity.get<StyleComponent>()->size;
        const auto entityShape = entity.get<StyleComponent>()->shape;
        const auto mousePos = input->mouse.position;

        const auto globalBounds = entityShape.getGlobalBounds();
        const auto globalMousePos = sf::Vector2f{ static_cast<float>(sf::Mouse::getPosition().x), static_cast<float>(sf::Mouse::getPosition().y) };
        const auto localMousePos = sf::Vector2f{ static_cast<float>(sf::Mouse::getPosition(*renderer.renderer).x), static_cast<float>(sf::Mouse::getPosition(*renderer.renderer).y) };

        const auto containsGlobal = globalBounds.contains(globalMousePos);
        const auto containsLocal = globalBounds.contains(localMousePos);
        const auto containsClient = globalBounds.contains(mousePos);

        return containsClient;
    }();

    if (mouseInsideRect) {
        spdlog::info("mouser inside {}, hovered: {}", entity.name(), hovered);
        if (!hovered && interaction.type != InteractionComponent::Type::Hovered) {
            interaction.type = InteractionComponent::Type::Hovered;
            return true;
        } else {
            if (hovered) {
                interaction.type = InteractionComponent::Type::None;
            }
        }
        return true;
    }

    interaction.type = InteractionComponent::Type::None;
    return false;
}