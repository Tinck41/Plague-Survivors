#include "application.h"
//#include "SFML/Graphics.hpp"
//#include "flecs.h"
//
//struct Position {
//	float x, y;
//};
//
//struct Velocity {
//	sf::Vector2f vel;
//};
//
//struct Player {
//	sf::RectangleShape someShape{};
//};
//
//int main()
//{
//	flecs::world ecs;
//
//	auto window = sf::RenderWindow(sf::VideoMode({800, 600}), "Plague: Survivors");
//	window.setFramerateLimit(144);
//
//	auto velocitySys = ecs.system<Position, const Velocity>()
//	.each([](flecs::iter& it, size_t, Position& p, const Velocity& v) {
//		sf::Vector2f direction { 0.f, 0.f };
//		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
//			direction.x = -1.f;
//		}
//		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
//			direction.x = 1.f;
//		}
//		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
//			direction.y = -1.f;
//		}
//		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
//			direction.y = 1.f;
//		}
//		if (direction.x != 0.f || direction.y != 0.f) {
//			const auto normalized = direction.normalized();
//			p.x += normalized.x * v.vel.x * it.delta_time();
//			p.y += normalized.y * v.vel.y * it.delta_time();
//		}
//	});
//	
//	auto positionSys = ecs.system<Player, const Position>()
//	.each([](Player& pl, const Position& pos) {
//		pl.someShape.setPosition({pos.x, pos.y});
//	});
//	
//	flecs::system playerSys = ecs.system<Player>()
//	.each([&window](Player& p) {
//		window.draw(p.someShape);
//	});
//	auto e = ecs.entity()
//	.set([](Position& p, Velocity& v, Player& pl) {
//		p = {10, 10};
//		v.vel = {500, 500};
//		pl.someShape.setSize({50, 50});
//		pl.someShape.setFillColor(sf::Color::White);
//	});
//
//	sf::RectangleShape shape{};
//	shape.setSize({100, 100});
//	shape.setPosition({100, 100});
//	shape.setFillColor(sf::Color::White);
//
//	sf::Clock deltaClock;
//	float deltaTime = 0.f;
//
//	while(ecs.progress(deltaTime) && window.isOpen()) {
//		for (auto event = sf::Event{}; window.pollEvent(event);)
//		{
//			if (event.type == sf::Event::Closed)
//			{
//				window.close();
//			}
//		}
//		deltaTime = deltaClock.restart().asSeconds();
//		window.clear();
//
//		velocitySys.run();
//		positionSys.run();
//		playerSys.run();
//
//		window.draw(shape);
//
//		window.display();
//	};
int main() {
	auto app = ps::core::Application::create();
	app->init();
	app->run();
	app->shutdown();
	return 0;
}
