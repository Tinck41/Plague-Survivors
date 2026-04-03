#include "core/application.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/default_modules.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/utils.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/windowModule/components.h"
#include "spdlog/spdlog.h"
#include "texture_atlas.h"

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

	world.system<Interaction, BackgroundColor>()
		.with<Button>()
		.kind(Phases::Update)
		.each([&](Interaction& interaction, BackgroundColor& color) {
			switch (interaction) {
				case Interaction::None: {
					color = RED;
					break;
				}
				case Interaction::Hovered: {
					color = GREEN;
					break;
				}
				case Interaction::Clicked: {
					color = BLUE;
					break;
				}
			}
		});
}

void init(flecs::world& world, ps::AssetStorage& storage, ps::RenderDevice& device) {
	using namespace ps;
	auto parent = world.entity("parent").add<ps::Node>();

	//world.entity("image")
	//	.set<ps::Image>({ .texture = storage.load_texture(*device.gpu, "assets/main_menu.jpg") })
	//	.child_of(parent);
	//world.entity("image2")
	//	.set<ps::Image>({ .texture = storage.load_texture(*device.gpu, "assets/main_menu.jpg") })
	//	.child_of(parent);

	auto sprite = world.entity("img")
		.set<Node>({ .size = { 300.f, 28.f } })
		.set<Image>({
			.texture = storage.load_texture(*device.gpu, "assets/atlas2x.png"),
			.texture_atlas = TextureAtlas::from_json("assets/atlas2x.json").value(),
		})
		.set<Composite>(Composite::create_3_h());

	auto text = world.entity("text")
		.set<Transform>({ .translation = {0.f, 0.f, 0.f } })
		.set<Text2d>({ "check some check for check" })
		.set<TextColor>(RED)
		.set<TextFont>({ storage.load_font("assets/FreeSans.ttf"), 16 });

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
	auto btn = world.entity("button")
		.child_of(parent)
		.set<ps::Transform>({ .translation = {200.f, 300.f, 0.f } })
		.set<ps::Node>({ .size = { 200, 200 } })
		.set<ps::BackgroundColor>(Color::from_hex("#00FF00"))
		.add<ps::Button>();
	world.entity("button2")
		.child_of(btn)
		.set<ps::Node>({ .size = { 100, 100 } })
		.set<ps::BackgroundColor>(RED)
		.add<ps::Button>()
		.set(ps::FocusStrategy::Block);

	BackgroundColor color = Color::from_hex("#00FF00");

	auto k = 0;
}

void init_2(flecs::world& world, ps::RenderDevice& device, ps::WindowModule& window_module) {
	using namespace ps;

//	auto window = world.entity("new_window")
//		.set<Window>(Window::create("new Window", glm::ivec2{ 100, 100 }));

	auto window = world.query_builder<Window>()
		.with<MainWindow>()
		.build()
		.first();

	auto camera = world.entity("camera")
		.set<ps::Transform>({ .translation = { 0.f, 0.f, 0.f } })
		.set<ps::RenderLayers>({ RenderLayers::layer(2) | RenderLayers::layer(1) })
		.add<ps::Camera>();


	//auto second_camera = world.entity("second_camera")
	//	.set<ps::Transform>({ .translation = { 100.f, 100.f, 0.f } })
	//	.set<ps::RenderLayers>({ RenderLayers::layer(2) | RenderLayers::layer(1) })
	//	.set<ps::Camera>({
	//			.render_target = std::make_shared<Texture>(
	//				device.gpu, glm::uvec2{ 500, 500 },
	//				TRANSPARENT,
	//				SDL_GetGPUSwapchainTextureFormat(device.gpu, window_module.main_window)
	//			),
	//		});

	//CameraCompositionGraph graph;
	//graph.add_edge(main_camera, second_camera);

	//auto res = graph.topological_sort();

	//world.set<CameraCompositionGraph>(graph);
	//world.add<CameraCompositionPipeline>();
};

int main() {
	ps::Application::create()
		.add_module<ps::DefaultModules>()
		.add_system(ps::Phases::OnStart, &init_2)
		.add_system(ps::Phases::OnStart, &init)
		.build_system(&button)
		//.add_script("assets/scripts/sandbox.flecs")
		.run();
}

