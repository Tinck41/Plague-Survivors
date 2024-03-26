#include "renderSystem.h"

#include "ecsModule/components/inputComponent.h"
#include "ecsModule/ecsController.h"
#include "ecsModule/types.h"
#include "ecsModule/components/spriteComponent.h"
#include "ecsModule/components/rendererComponent.h"

using namespace ps;
using namespace ecsModule;

void RenderSystem::create() {
	auto& world = EcsControllerInstance::getInstance()->getWorld();

	world.system<RendererComponent>()
		.term_at(1).singleton()
		.kind(Phases::Clear)
		.each([](RendererComponent& r) {
			r.renderer->clear();
		});

	world.system<RendererComponent, SpriteComponent>()
		.term_at(1).singleton()
		.kind(Phases::Render)
		.each([](flecs::iter& it, size_t, RendererComponent& r, SpriteComponent& s) {
			r.renderer->draw(s.sprite);
		});
	
	world.system<RendererComponent>()
		.term_at(1).singleton()
		.kind(Phases::Display)
		.each([](RendererComponent& r) {
			r.renderer->display();
		});
	
	world.system<RendererComponent, InputComponent>()
		.term_at(1).singleton()
		.term_at(2).singleton()
		.kind(Phases::Update)
		.each([](RendererComponent& r, InputComponent& i) {
			if (i.keys[Key::Q].pressed) {
				r.renderer->close();
			}
		});

	world.set<RendererComponent>(
		{ std::make_unique<sf::RenderWindow>(sf::VideoMode({ 600, 600 }), "Plague: Survivors") }
	);
}
