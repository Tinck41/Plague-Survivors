#include "module.h"

#include "SFML/Graphics/Color.hpp"
#include "ecsModule/appModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/transformModule/module.h"
#include "resourceManager.h"

using namespace ps;

RenderModule::RenderModule(flecs::world& world) {
	world.module<RenderModule>();

	world.import<AppModule>();
	world.import<TransformModule>();

	world.component<sf::Color>()
		.member<std::uint8_t>("r")
		.member<std::uint8_t>("g")
		.member<std::uint8_t>("b")
		.member<std::uint8_t>("a");

	world.component<Sprite>()
		.member<sf::Color>("color")
		.add(flecs::With, world.component<Transform>());
	world.component<Rectangle>().add(flecs::With, world.component<Transform>());
	world.component<Circle>().add(flecs::With, world.component<Transform>());

	//world.add<RenderQueue>();

	world.observer<Sprite>()
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Sprite& s) {
			e.emplace<RenderFunc>([](sf::RenderWindow& window, flecs::entity e) {
				auto s = e.get<Sprite>();
				sf::Sprite sprite{ *s->texture };
				sprite.setColor(s->color);
				window.draw(sf::Sprite(sprite));
			});
		});

	world.observer<Rectangle>()
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Rectangle& r) {
			e.emplace<RenderFunc>([](sf::RenderWindow& window, flecs::entity e) {
				const auto r = e.get<Rectangle>();
				window.draw(sf::RectangleShape(r->size));
			});
		});

	world.observer<Circle>()
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Circle& c) {
			e.emplace<RenderFunc>([](sf::RenderWindow& window, flecs::entity e) {
				const auto c = e.get<Circle>();
				window.draw(sf::CircleShape(c->radius));
			});
		});

	//world.observer<Sprite>()
	//	.event(flecs::OnSet)
	//	.each([](flecs::entity e, Sprite& s) {
	//		if (!s.texture) {
	//			s.texture = std::make_shared<sf::Texture>();
	//		}
	//		if (!s.sprite) {
	//			s.sprite = new sf::Sprite(*s.texture);
	//		}

	//		s.sprite->setColor(s.color);

	//		if (s.size) {
	//			s.sprite->setTextureRect(sf::IntRect{{ 0, 0, }, { static_cast<int>(s.size.value().x), static_cast<int>(s.size.value().y) } } );
	//		}

	//		e.set<RenderItem>({ s.sprite, s.states });
	//	});

	//world.observer<Rectangle>()
	//	.event(flecs::OnSet)
	//	.each([](flecs::entity e, Rectangle& r) {
	//		e.emplace<RenderItem>(new sf::RectangleShape(r.size));
	//	});

	//world.observer<Circle>()
	//	.event(flecs::OnSet)
	//	.each([](flecs::entity e, Circle& c) {
	//		e.emplace<RenderItem>(new sf::CircleShape(c.radius));
	//	});

	//world.system<RenderQueue>()
	//	.term_at(0).singleton()
	//	.kind(Phases::PreUpdate)
	//	.each([](RenderQueue& queue) {
	//		queue.clear();
	//	});

	//world.system<RenderQueue, Sprite>()
	//	.term_at(0).singleton()
	//	.kind(Phases::PostUpdate)
	//	.each([](flecs::entity e, RenderQueue& queue, Sprite& sprite) {
	//		queue.emplace(e.depth(flecs::ChildOf), &sprite.sprite);
	//	});

	//world.system<RenderQueue, Rectangle>()
	//	.term_at(0).singleton()
	//	.kind(Phases::PostUpdate)
	//	.each([](flecs::entity e, RenderQueue& queue, Rectangle& rect) {
	//		queue.emplace(e.depth(flecs::ChildOf), &rect);
	//	});

	//world.system<RenderQueue, Circle>()
	//	.term_at(0).singleton()
	//	.kind(Phases::PostUpdate)
	//	.each([](flecs::entity e, RenderQueue& queue, Circle& circle) {
	//		queue.emplace(e.depth(flecs::ChildOf), &circle);
	//	});

	world.system<Application>()
		.term_at(0).singleton()
		.kind(Phases::Clear)
		.each([](Application& app) {
			app.window.clear();
		});

	//world.system<Application, const RenderItem, const GlobalTransform>()
	//	.term_at(0).singleton()
	//	.term_at(1).self().cascade().cached()
	//	.kind(Phases::Render)
	//	.order_by<GlobalTransform>([](flecs::entity_t e1, const GlobalTransform *t1, flecs::entity_t e2, const GlobalTransform *t2) {
	//		return (t1->translation.z > t2->translation.z) - (t1->translation.z < t2->translation.z);
	//	})
	//	.each([](Application& app, const RenderItem& r, const GlobalTransform& t) {
	//		app.window.draw(*r.item, r.states);
	//	});


	world.system<Application, const RenderFunc, const GlobalTransform>()
		.term_at(0).singleton()
		.term_at(2).self().cascade().cached()
		.kind(Phases::Render)
		.order_by<GlobalTransform>([](flecs::entity_t e1, const GlobalTransform *t1, flecs::entity_t e2, const GlobalTransform *t2) {
			return (t1->translation.z > t2->translation.z) - (t1->translation.z < t2->translation.z);
		})
		.each([](flecs::entity e, Application& app, const RenderFunc& r, const GlobalTransform& t) {
			r(app.window, e);
		});

	world.system<Application>()
		.term_at(0).singleton()
		.kind(Phases::Display)
		.each([](Application& app) {
			app.window.display();
		});

	// Sandbox
	auto e1 = world.entity()
		.emplace<Sprite>(ResourceManager::getInstance()->getTexture("assets/main_menu.jpg"));

	e1.get_ref<Transform>()->translation.z = 1.f;

	auto e2 = world.entity()
		.set<Circle>({300.f});

	auto e3 = world.entity()
		.set<Rectangle>({ sf::Vector2f{100.f, 100.f} });

	e2.child_of(e1);

	//auto e4 = world.entity()
	//	.emplace<Sprite>(nullptr, sf::Color::Red, Vec2f{ 100.f, 100.f });
}
