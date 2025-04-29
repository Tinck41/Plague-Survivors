#include "module.h"

#include "SFML/Graphics/Sprite.hpp"
#include "ecsModule/common.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/appModule/module.h"
#include <format>
#include <functional>
#include <iostream>

using namespace ps;

int depth = 0;

sf::Font defaultFont;

void dfs(flecs::entity e, std::function<void(flecs::entity)> callback) {
	callback(e);

	e.children([callback](flecs::entity child) {
		dfs(child, callback);
	});
}

UiModule::UiModule(flecs::world& world) {
	world.module<UiModule>();

	world.component<RootNode>().add(flecs::Exclusive);
	world.component<Node>();
	//world.component<Anchor>();
	//world.component<Pivot>();
	world.component<UiTraversalId>();
	world.component<Style>();
	world.component<Interaction>();
	world.component<Image>();
	world.component<Text>();

	//world.component<Anchor>()
	//	.member<float>("x")
	//	.member<float>("y");

	//world.component<Pivot>()
	//	.member<float>("x")
	//	.member<float>("y");


	if (!defaultFont.openFromFile("assets/FreeSans.ttf")) {
		std::cout << "can't load default font\n";
	}

	auto root = world.entity(UI_ROO_ID).add<Node>().add<RootNode>().add<Transform>();

	world.observer<Style>()
		.event(flecs::OnSet)
		.each([](Style& s) {
			s.shape.setFillColor(s.backgroundColor);
		});

	world.observer<Node>()
		.event(flecs::OnSet)
		.each([&root](flecs::iter& it, size_t size, Node node) {
			root.add<Dirty>();
			depth = 0;
		});

	world.system<RootNode, Dirty>()
		.kind(Phases::Update)
		.each([world](flecs::entity e, RootNode, Dirty) {
			e.remove<Dirty>();

			dfs(e, [](flecs::entity e) {
				e.set<UiTraversalId>(depth++);
			});
		});

	//world.system<Style*, Transform, Anchor*, Pivot*, Style>()
	//	.term_at(0).parent().cascade()
	//	.term_at(1).second<Global>()
	//	.with<Node>()
	//	.with<Dirty>()
	//	.kind(Phases::Update)
	//	.each([](Style* parentS, Transform& childT, Anchor* childA, Pivot* childP, Style& childS) {
	//		if (parentS) {
	//			if (childA) {
	//				childT.translation.x += parentS->size.x * childA->x;
	//				childT.translation.y += parentS->size.y * childA->y;
	//			}
	//		}
	//		if (childP) {
	//			childT.translation.x += childS.size.x * childP->x;
	//			childT.translation.y += childS.size.y * childP->y;
	//		}
	//	});

	world.system<const Input, Interaction, const Style, const UiTraversalId>()
		.with<Node>()
		.term_at(0).singleton()
		.kind(Phases::Update)
		.order_by<UiTraversalId>([](flecs::entity_t e1, const UiTraversalId *d1, flecs::entity_t e2, const UiTraversalId *d2) {
			return (*d1 < *d2) - (*d1 > *d2);
		})
		.run([](flecs::iter& it) {
			auto hovered = false;

			while (it.next()) {
				auto input       = it.field<const Input>(0);
				auto interaction = it.field<Interaction>(1);
				auto style       = it.field<const Style>(2);

				for (auto i : it ) {
					const auto mouseInsideRect = [&] {
						const auto entityShape = style[i].shape;
						const auto mousePos = input->mouse.position;

						const auto globalBounds = entityShape.getGlobalBounds();

						const auto containsClient = globalBounds.contains(mousePos);

						return containsClient;
					}();

					if (mouseInsideRect) {
						//spdlog::info("mouser inside {}, hovered: {}", entity.name(), hovered);
						if (!hovered && interaction[i].type != Interaction::Type::Hovered) {
							interaction[i].type = Interaction::Type::Hovered;
							hovered = true;
						} else {
							if (hovered) {
								interaction[i].type = Interaction::Type::None;
							} else {
								hovered = true;
							}
						}
					} else {
						interaction[i].type = Interaction::Type::None;
					}
				}
			}
		});

	world.system<const Interaction, Style>()
		.with<Node>()
		.kind(Phases::Update)
		.each([](const Interaction& i, Style& s) {
			switch (i.type) {
				case Interaction::Type::Hovered:
					//spdlog::info("{} is hovered", entity.name());
					s.shape.setFillColor(sf::Color::Red);
					break;
				case Interaction::Type::Clicked:
					s.shape.setFillColor(sf::Color::Green);
					break;
				case Interaction::Type::None:
					s.shape.setFillColor(s.backgroundColor);
					break;
				default:
					break;
			}
		});

	world.system<Style, const GlobalTransform>()
		.with<Node>()
		.kind(Phases::Update)
		.each([](Style& s, const GlobalTransform& t) {
			s.shape.setPosition({ t.translation.x , t.translation.y });
		});

	world.system<Application, Style*, Image*, Text*, const UiTraversalId>()
		.with<Node>()
		.term_at(0).singleton()
		.kind(Phases::Render)
		.order_by<UiTraversalId>([](flecs::entity_t e1, const UiTraversalId *d1, flecs::entity_t e2, const UiTraversalId *d2) {
			return (*d1 > *d2) - (*d1 < *d2);
		})
		.each([](Application& app, Style* s, Image* i, Text* t, UiTraversalId) {
			if (s) {
				app.window.draw(s->shape);
			}
			if (i) {
				app.window.draw(sf::Sprite(i->texture));
			}
			if (t) {
				app.window.draw(*t->text);
			}
		});

	const auto windowSize = [&]() {
		const auto windowSize = world.get<Application>()->window.getSize();
		return sf::Vector2f { static_cast<float>(windowSize.x), static_cast<float>(windowSize.y) };
	}();
	const auto canvas = world.entity("canvas")
		.child_of(root)
		.add<Node>()
		.set(Transform{})
		.set(Image{
			.texture{ "assets/main_menu.jpg" },
		})
		.set(Style {
			.size = windowSize,
			.backgroundColor = sf::Color::Blue,
			.shape = sf::RectangleShape{ windowSize }
		});

	auto button = world.entity("button")
		.child_of(canvas)
		.add<Node>()
		.add<Interaction>()
		.set(Style {
			.size = { 200.f, 60.f },
			.backgroundColor = sf::Color::Cyan,
			.shape = sf::RectangleShape{{ 200.f, 60.f }}
		})
		.set(Transform {
			.translation = {10.f, 0.f }
		});

	auto button2 = world.entity("button2")
		.child_of(canvas)
		.add<Node>()
		.set(Style {
			.size = { 200.f, 60.f },
			.backgroundColor = sf::Color::Cyan,
			.shape = sf::RectangleShape{{ 200.f, 60.f }}
		})
		.set(Transform {
			.translation = {10.f, 80.f }
		});
	auto button3 = world.entity("button3")
		.child_of(button2)
		.add<Node>()
		.add<Interaction>()
		.set(Style {
			.size = { 50.f, 50.f },
			.backgroundColor = sf::Color::Cyan,
			.shape = sf::RectangleShape{{ 50.f, 50.f }}
		})
		.set(Transform {
			.translation = { 210.f, 80.f }
		});
	auto button4 = world.entity("button4")
		.child_of(button2)
		.add<Node>()
		.add<Interaction>()
		.set(Style {
			.size = { 100.f, 100.f },
			.backgroundColor = sf::Color::Black,
			.shape = sf::RectangleShape{{ 100.f, 100.f }}
		})
		.set(Transform {
			.translation = { 210.f, 90.f }
		});
		sf::Text text{ defaultFont, "", 32u };
	auto button5 = world.entity("button5")
		.child_of(button4)
		.add<Node>()
		.set(Text{
			.text = new sf::Text{ defaultFont, "hello", 32u }
		})
		.set(Transform {
			.translation = { 210.f, 80.f }
		})
		.add<Dirty>();
}
