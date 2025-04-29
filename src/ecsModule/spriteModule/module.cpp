#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/appModule/module.h"
#include "ecsModule/transformModule/module.h"

using namespace ps;

SpriteModule::SpriteModule(flecs::world& world) {
	//world.module<SpriteModule>();

	//world.component<Sprite>();

	//world.system<Sprite, const Transform>()
	//	.term_at(1).second<Global>()
	//	.kind(Phases::Update)
	//	.each([](Sprite& s, const Transform& t) {
	//		s.sprite.setPosition(t.translation);
	//	});

	//world.system<Application, Sprite>()
	//	.term_at(0).singleton()
	//	.kind(Phases::Render)
	//	.each([](Application& app, Sprite& s) {
	//		app.window.draw(s.sprite);
	//	});
}
