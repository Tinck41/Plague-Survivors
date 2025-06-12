#include "module.h"

#include "SDL3_image/SDL_image.h"
#include "SDL3/SDL.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/appModule/module.h"
#include "ext/matrix_clip_space.hpp"
#include "spdlog/spdlog.h"
#include "utils/sdl.h"

using namespace ps;

RenderModule::RenderModule(flecs::world& world) {
	world.module<RenderModule>();

	world.import<AppModule>();
	world.import<TransformModule>();

	world.component<Renderer>();
	world.component<SpritePipeline>();
	world.component<Drawable>();
	world.component<Sprite>()
		.add(flecs::With, world.component<Drawable>())
		.add(flecs::With, world.component<Transform>());
	world.component<Rectangle>()
		.add(flecs::With, world.component<Drawable>())
		.add(flecs::With, world.component<Transform>());
	world.component<Circle>()
		.add(flecs::With, world.component<Drawable>())
		.add(flecs::With, world.component<Transform>());

	world.component<SDL_Color>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a");

	world.component<std::string>()
		.opaque(flecs::String)
			.serialize([](const flecs::serializer *s, const std::string *data) {
				const char *str = data->c_str();
				return s->value(flecs::String, &str);
			})
			.assign_string([](std::string* data, const char *value) {
				*data = value;
			});

	world.observer<Sprite>()
		.event(flecs::OnAdd)
		.each([](Sprite& s) {
			// TODO: load sprite via AssetStorage from AssetModule
		});

	world.system<Application, Renderer>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(Phases::OnStart)
		.each([](Application& app, Renderer& renderer) {
			auto gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, NULL);

			if (!gpu) {
				print_sdl_error();

				return;
			}

			auto result = SDL_ClaimWindowForGPUDevice(gpu, app.window); //assert(result);

			if (!result) {
				print_sdl_error();

				return;
			}

			int width;
			int height;
			SDL_GetWindowSize(app.window, &width, &height);

			renderer.gpu = gpu;
			renderer.projection = glm::perspective(70.f, static_cast<float>(width) / static_cast<float>(height), 0.0001f, 1000.f);
		});

	world.system<Application, Renderer, SpritePipeline>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.term_at(2).singleton()
		.kind(Phases::OnStart)
		.each([](Application& app, Renderer& r, SpritePipeline& sp) {
			auto vert_shader = load_shader(*r.gpu, "assets/shaders/out/shader.vert.msl", 1);
			auto frag_shader = load_shader(*r.gpu, "assets/shaders/out/shader.frag.msl");

			std::array<internal::VertexData, 4> verticies{
				internal::VertexData{ glm::vec3{ -0.5f,  0.5f, -5.f }, glm::vec4{ 1, 0, 0, 1 } },
				internal::VertexData{ glm::vec3{  0.5f,  0.5f, -5.f }, glm::vec4{ 1, 0, 0, 1 } },
				internal::VertexData{ glm::vec3{ -0.5f, -0.5f, -5.f }, glm::vec4{ 0, 1, 0, 1 } },
				internal::VertexData{ glm::vec3{  0.5f, -0.5f, -5.f }, glm::vec4{ 0, 0, 1, 1 } }
			};

			std::array<uint16_t, 6> indecies{
				0, 1, 2,
				2, 3, 1
			};

			const auto verticies_byte_szie = verticies.size() * sizeof(verticies[0]);
			const auto indecies_byte_szie = indecies.size() * sizeof(indecies[0]);

			auto vertex_buf = SDL_CreateGPUBuffer(r.gpu, new SDL_GPUBufferCreateInfo{
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = verticies_byte_szie,
			});

			auto index_buf = SDL_CreateGPUBuffer(r.gpu, new SDL_GPUBufferCreateInfo{
				.usage = SDL_GPU_BUFFERUSAGE_INDEX,
				.size = indecies_byte_szie,
			});

			auto transfer_buf = SDL_CreateGPUTransferBuffer(r.gpu, new SDL_GPUTransferBufferCreateInfo{
				.usage = SDL_GPUTransferBufferUsage::SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<uint32_t>(verticies_byte_szie + indecies_byte_szie)
			});

			auto transfer_mem = SDL_MapGPUTransferBuffer(r.gpu ,transfer_buf, false);

			std::memcpy(transfer_mem, verticies.data(), verticies_byte_szie);
			std::memcpy(static_cast<char*>(transfer_mem) + verticies_byte_szie, indecies.data(), indecies_byte_szie);

			SDL_UnmapGPUTransferBuffer(r.gpu, transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(r.gpu);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

			SDL_UploadToGPUBuffer(copy_pass,
				new SDL_GPUTransferBufferLocation{ .transfer_buffer = transfer_buf },
				new SDL_GPUBufferRegion{ .buffer = vertex_buf, .size = static_cast<uint32_t>(verticies_byte_szie) },
				false
			);

			SDL_UploadToGPUBuffer(copy_pass,
				new SDL_GPUTransferBufferLocation{ .transfer_buffer = transfer_buf, .offset = static_cast<uint32_t>(verticies_byte_szie) },
				new SDL_GPUBufferRegion{ .buffer = index_buf, .size = static_cast<uint32_t>(indecies_byte_szie) },
				false
			);

			SDL_EndGPUCopyPass(copy_pass);

			auto result = SDL_SubmitGPUCommandBuffer(copy_cmd_buf);

			std::array<SDL_GPUVertexAttribute, 2> vertex_attrs{
				SDL_GPUVertexAttribute{
					.location = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = offsetof(internal::VertexData, position),
				},
				SDL_GPUVertexAttribute{
					.location = 1,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = offsetof(internal::VertexData, color),
				}
			};

			SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{
				.vertex_shader = vert_shader,
				.fragment_shader = frag_shader,
				.vertex_input_state = {
					.vertex_buffer_descriptions = new SDL_GPUVertexBufferDescription{
						.slot = 0,
						.pitch = sizeof(internal::VertexData),
					},
					.num_vertex_buffers = 1,
					.vertex_attributes = vertex_attrs.data(),
					.num_vertex_attributes = vertex_attrs.size(),
				},
				.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				.rasterizer_state = {
					.cull_mode = SDL_GPU_CULLMODE_NONE,
					.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
				},
				.target_info = {
					.color_target_descriptions = new SDL_GPUColorTargetDescription{
						.format = SDL_GetGPUSwapchainTextureFormat(r.gpu, app.window)
					},
					.num_color_targets = 1,
				}
			};

			auto pipeline = SDL_CreateGPUGraphicsPipeline(r.gpu, &pipeline_create_info);

			if (!pipeline) {
				print_sdl_error();

				return;
			}

			SDL_ReleaseGPUShader(r.gpu, vert_shader);
			SDL_ReleaseGPUShader(r.gpu, frag_shader);

			sp.pipeline = pipeline;
			sp.index_buffer = index_buf;
			sp.vertex_buffer = vertex_buf;
		});

	world.system<Application, Renderer, SpritePipeline>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.term_at(2).singleton()
		.kind(Phases::Render)
		.each([](Application& app, Renderer& renderer, SpritePipeline& sp) {
			auto cmd_buf = SDL_AcquireGPUCommandBuffer(renderer.gpu);

			SDL_GPUTexture* render_target;

			auto result = SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, app.window, &render_target, nullptr, nullptr); assert(result);

			auto color_target = SDL_GPUColorTargetInfo{
				.texture = render_target,
				.clear_color = { 0.f, 1.f, 1.f, 1.f },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE
			};
			auto render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, nullptr);

			internal::UBO ubo{ 
				.mvp = renderer.projection
			};
			SDL_PushGPUVertexUniformData(cmd_buf, 0, &ubo, sizeof(ubo));

			SDL_BindGPUGraphicsPipeline(render_pass, sp.pipeline);
			SDL_BindGPUVertexBuffers(render_pass, 0, new SDL_GPUBufferBinding{ .buffer = sp.vertex_buffer }, 1);
			SDL_BindGPUIndexBuffer(render_pass, new SDL_GPUBufferBinding{ .buffer = sp.index_buffer }, SDL_GPU_INDEXELEMENTSIZE_16BIT);

			SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);

			SDL_EndGPURenderPass(render_pass);

			result &= SDL_SubmitGPUCommandBuffer(cmd_buf); assert(result);
		});

	world.system()
		.kind(Phases::OnStart)
		.each([](flecs::iter& it, size_t i) {
		auto world = it.world();
		world.entity()
			.add<Dirty>()
			.set<Rectangle>({
				.size{ 100, 100},
				.color{ 0, 1, 0, 1 }
			})
			.set<Transform>({
				.rotation{ 15, 0, 0 }
			});

		world.entity()
			.add<Dirty>()
			.set<Sprite>({
				.texture = world.get_ref<AssetStorage>()->load_texture("assets/main_menu.jpg")
			});
		});

	world.add<Renderer>();
	world.add<SpritePipeline>();
}
