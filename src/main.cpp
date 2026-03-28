#include "core/application.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/default_modules.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/utils.h"
#include "ecsModule/ui_module/module.h"
#include "spdlog/spdlog.h"

void button(flecs::world& world) {
	using namespace ps;

	world.system<Input>()
		.kind(Phases::Update)
		.each([&world](Input& input) {
			if (input.keys[Key::F].pressed) {
				world.query<Text2d>().each([](flecs::entity e, Text2d&) {
					e.set<Text2d>("new text");
				});
			}
		});

	const BackgroundColor defaultColor = RED;
	const BackgroundColor hoverColor = GREEN;
	const BackgroundColor clickColor = BLUE;

	world.system<Interaction, BackgroundColor>()
		.with<Button>()
		.kind(Phases::Update)
		.each([&](Interaction& interaction, BackgroundColor& color) {
			switch (interaction) {
				case Interaction::None: {
					color = defaultColor;
					break;
				}
				case Interaction::Hovered: {
					color = hoverColor;
					break;
				}
				case Interaction::Clicked: {
					color = clickColor;
					break;
				}
			}
		});
}

void init(flecs::world& world, ps::AssetStorage& storage, ps::RenderDevice& device) {
	using namespace ps;
	//auto parent = world.entity("parent").add<ps::Node>();

	CameraModule::EcsCamera = world.entity("EcsCamera").add<Camera>();

	//world.entity("image")
	//	.set<ps::Image>({ .texture = storage.load_texture(*device.gpu, "assets/main_menu.jpg") })
	//	.child_of(parent);
	//world.entity("image2")
	//	.set<ps::Image>({ .texture = storage.load_texture(*device.gpu, "assets/main_menu.jpg") })
	//	.child_of(parent);

	auto sprite = world.entity("sprite")
		.set<ps::Sprite>({ .texture = storage.load_texture(*device.gpu, "assets/COUPON.png") });

	auto text = world.entity("text")
		.set<Transform>({ .translation = {100.f, 0.f, 0.f } })
		.set<Text2d>({ "check some check for check" })
		.set<TextColor>(WHITE)
		.set<TextFont>({ storage.load_font("assets/FreeSans.ttf", 16) });

	//auto child = world.entity("child").add<ps::Node>().child_of(parent);
	//auto child2 = world.entity("child2").emplace<ps::Node>().emplace<ps::CustomNodeIndex>(0u).child_of(parent);

	//world.entity("sprite3")
	//	.set<ps::Transform>({ .translation = {300.f, 0.f, 2.f } })
	//	.set<ps::Sprite>({ .texture = storage.load_texture(*device.gpu, "assets/COUPON.png") });
	//world.entity("sprite2")
	//	.set<ps::Transform>({ .translation = {100.f, 0.f, 1.f } })
	//	.set<ps::Sprite>({ .texture = storage.load_texture(*device.gpu, "assets/main_menu.jpg") });
	//world.entity("sprite4")
	//	.set<ps::Transform>({ .translation = {200.f, 0.f, 1.f } })
	//	.set<ps::Sprite>({ .texture = storage.load_texture(*device.gpu, "assets/main_menu.jpg") });
	//world.entity("text")
	//	.set<ps::Text>({ .string = "\nHello\nSDL3_ttf!", .font = storage.load_font("assets/FreeSans.ttf", 46) });
	//world.entity("text2")
	//	.set<ps::Text>({ .string = "check some check for check!", .font = storage.load_font("assets/FreeSans.ttf", 46) });
	//
	//auto btn = world.entity("button")
	//	.child_of(parent)
	//	.set<ps::Transform>({ .translation = {200.f, 300.f, 0.f } })
	//	.set<ps::Node>({ .size = { 200, 200 } })
	//	.add<ps::Button>();
	//world.entity("button2")
	//	.child_of(btn)
	//	.set<ps::Node>({ .size = { 100, 100 } })
	//	.emplace<ps::BackgroundColor>(1.f, 0.f, 0.f, 1.f)
	//	.add<ps::Button>()
	//	.set(ps::FocusStrategy::Block);
}

int main() {
	ps::Application::create()
		.add_module<ps::DefaultModules>()
		.add_system(ps::Phases::OnStart, &init)
		.build_system(&button)
		//.add_script("assets/scripts/sandbox.flecs")
		.run();
}

