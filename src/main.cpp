#include "core/application.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/default_modules.h"
#include "ecsModule/inputModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/ui_render_module_new/components.h"
#include "ecsModule/ui_render_module_new/module.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/utils.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/ui_render_module_new/text_helpers.h"
#include "ecsModule/ui_render_module_new/node_helpers.h"
#include "ecsModule/windowModule/components.h"
#include "spdlog/spdlog.h"
#include "texture_atlas.h"
#include <algorithm>
#include "ecsModule/render_module/module.h"
#include "ecsModule/sprite_render_module/module.h"
#include "ecsModule/text_render_module/module.h"
#include "utils/sdl.h"

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
			//.texture_atlas = TextureAtlas::from_json("assets/atlas2x.json").value(),
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

void init_2(flecs::world& world, ps::RenderDevice& device, ps::AssetStorage& storage) {
	using namespace ps;

//	auto window = world.entity("new_window")
//		.set<Window>(Window::create("new Window", glm::ivec2{ 100, 100 }));

	//auto window = world.query_builder<Window>()
	//	.with<MainWindow>()
	//	.build()
	//	.first();
	auto text = world.entity("text")
		.set<Transform>({ .translation = {0.f, 0.f, 0.f } })
		.set<Text2d>({ "check some\n check for\n check" })
		.set<TextColor>(RED)
		.set<TextOutline>({
			.width = 1.f,
			.color = WHITE
		})
		.set<TextFont>({ storage.load_font("assets/FreeSans.ttf"), 16, 16 });

	world.entity("sprite2")
		.set<ps::Transform>({ .translation = {300.f, 0.f, 2.f } })
		.set<ps::Sprite>({ .texture = storage.load_texture(*device.gpu, "assets/COUPON.png") });

	auto texture = storage.load_texture(*device.gpu, "assets/minecraft.png");
	auto atlas = TextureAtlas{
		.current_index = 150,
	};

	glm::vec2 size = texture->get_size() / 16.f;

	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			atlas.rects.emplace_back(size.x * x, size.y * y, size.x ,size.y);
		}
	}

	auto sprite3 = world.entity("sprite3")
		.set<ps::Transform>({ .translation = {300.f, 0.f, 2.f } })
		.set<ps::Sprite>({
			.texture_atlas = atlas,
			.texture = texture,
		});

	auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/test.vert.msl", 1);
	auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/test.frag.msl", 1, 1);

	SDL_GPUColorTargetDescription color_target_description{
		.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, world.get<WindowModule>().main_window),
		.blend_state = {
			.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.color_blend_op = SDL_GPU_BLENDOP_ADD,
			.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
			.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
			.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
			.enable_blend = true,
		}
	};

	std::array<SDL_GPUVertexAttribute, 6> vertex_attrs{
		SDL_GPUVertexAttribute{
			.location = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(SpriteInstance, translation),
		},
		SDL_GPUVertexAttribute{
			.location = 1,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(SpriteInstance, rotation),
		},
		SDL_GPUVertexAttribute{
			.location = 2,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(SpriteInstance, scale),
		},
		SDL_GPUVertexAttribute{
			.location = 3,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
			.offset = offsetof(SpriteInstance, color),
		},
		SDL_GPUVertexAttribute{
			.location = 4,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(SpriteInstance, uv),
		},
		SDL_GPUVertexAttribute{
			.location = 5,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
			.offset = offsetof(SpriteInstance, size),
		},
	};

	SDL_GPUVertexBufferDescription vertex_buffer_description{
		.slot = 0,
		.pitch = sizeof(SpriteInstance),
		.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
	};

	SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{
		.vertex_shader = vert_shader,
		.fragment_shader = frag_shader,
		.vertex_input_state = {
			.vertex_buffer_descriptions = &vertex_buffer_description,
			.num_vertex_buffers = 1,
			.vertex_attributes = vertex_attrs.data(),
			.num_vertex_attributes = vertex_attrs.size(),
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.target_info = {
			.color_target_descriptions = &color_target_description,
			.num_color_targets = 1,
		}
	};

	SDL_GPUSamplerCreateInfo sampler_create_info{};

	auto sampler = SDL_CreateGPUSampler(device.gpu, &sampler_create_info);
	auto pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

	SDL_ReleaseGPUShader(device.gpu, vert_shader);
	SDL_ReleaseGPUShader(device.gpu, frag_shader);

	sprite3
		.set<Material>({
			.pipeline = pipeline,
			.sampler = sampler,
			.vertex_uniforms = { world.id<DefaultUniform>() },
			.fragment_uniforms = { world.id<CirlceUniform>() },
		})
		.set<CirlceUniform>({ .radius = 15.f });

	auto img = world.entity("img")
		.set<Image>({
			.texture = storage.load_texture(*device.gpu, "assets/main_menu.jpg"),
		});

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
	//	transparent_2d
	//		extract sprites
	//		extract texts 
	//
	//		render all

	//CameraCompositionGraph graph;
	//graph.add_edge(main_camera, second_camera);

	//auto res = graph.topological_sort();

	//world.set<CameraCompositionGraph>(graph);
	//world.add<CameraCompositionPipeline>();


	//auto transparent_ui = world.entity("transparent_ui");
};

void default_init(flecs::world& world) {
	using namespace ps;

	auto camera = world.entity("camera")
		.set<ps::Transform>({ .translation = { 0.f, 0.f, 0.f } })
		.set<ps::RenderLayers>({ RenderLayers::layer(2) | RenderLayers::layer(1) })
		.add<ps::Camera>();

	{
		auto transparent_2d = world.entity("transparent_2d");
		auto transparent_ui = world.entity("transparent_ui");

		{
			auto sprite_phase = world.entity("sprite_phase").child_of(transparent_2d);
			auto text_phase = world.entity("text_phase").child_of(transparent_2d);

			sprite_phase.set(create_sprite_material(world));
			sprite_phase.set(create_sprite_context(world));

			text_phase.set(create_text_material(world));
			text_phase.set(create_text_context(world));

			sprite_phase.set(create_sprite_extractor(world, sprite_phase));
			sprite_phase.set(create_sprite_uploader());

			text_phase.set(create_text_extractor(world, text_phase));
			text_phase.set(create_text_uploader());

			sprite_phase.add<ExtractedSprites>();
			text_phase.add<ExtractedText2ds>();

			transparent_2d.set(create_default_sorter());
			transparent_2d.set(create_default_renderer());

			transparent_2d.set(RenderPhase{
				.extractors = { sprite_phase, text_phase },
				.sorters = { transparent_2d },
				.uploaders = { sprite_phase, text_phase },
				.renderers = { transparent_2d },
			});
		}

		{
			auto node_helper = world.entity("node_helper").child_of(transparent_ui);
			auto text_node_helper = world.entity("text_node_helper").child_of(transparent_ui);
			auto image_extractor = world.entity("image_extractor").child_of(node_helper);
			auto color_extractor = world.entity("color_extractor").child_of(node_helper);

			node_helper.add<ExtractedNodes>();
			text_node_helper.add<ExtractedTextNodes>();

			node_helper.set(create_node_context(world));
			text_node_helper.set(create_text_context(world));

			transparent_ui.set(create_default_sorter());
			transparent_ui.set(create_default_renderer());

			node_helper.set(create_node_material(world));
			text_node_helper.set(create_text_material(world));

			image_extractor.set(create_image_node_extractor(world, node_helper));
			color_extractor.set(create_color_node_extractor(world, node_helper));
			text_node_helper.set(create_text_node_extractor(world, text_node_helper));

			node_helper.set(create_node_uploader());
			text_node_helper.set(create_text_node_uploader());

			transparent_ui.set(RenderPhase{
				.extractors = { image_extractor, color_extractor, text_node_helper },
				.sorters = { transparent_ui },
				.uploaders = { node_helper, text_node_helper },
				.renderers = { transparent_ui },
				.extract_view_callback = &extract_ui_view,
			});
		}

		camera.set(RenderPhases{ transparent_2d, transparent_ui });
	}
}

int main() {
	ps::Application::create()
		.add_module<ps::DefaultModules>()
		.add_system(ps::Phases::OnStart, &default_init)
		.add_system(ps::Phases::OnStart, &init_2)
		//.build_system(&button)
		//.add_script("assets/scripts/sandbox.flecs")
		.run();
}

