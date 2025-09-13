#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/appModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/meshModule/module.h"
#include "ext/matrix_transform.hpp"
#include "spdlog/spdlog.h"
#include "utils/sdl.h"

using namespace ps;

SpriteModule::SpriteModule(flecs::world& world) {
	world.module<SpriteModule>();

	world.import<AppModule>();
	world.import<RenderModule>();
	world.import<MeshModule>();
	world.import<TransformModule>();

	world.component<SpritePipeline>().add(flecs::Singleton);
	world.component<SpriteRenderer>().add(flecs::Singleton);
	world.component<Sprite>()
		.add(flecs::With, world.component<Transform>());

	world.component<std::string>()
		.opaque(flecs::String)
			.serialize([](const flecs::serializer *s, const std::string *data) {
				const char *str = data->c_str();
				return s->value(flecs::String, &str);
			})
			.assign_string([](std::string* data, const char *value) {
				*data = value;
			});

	world.component<SDL_FColor>()
		.member<float>("r")
		.member<float>("g")
		.member<float>("b")
		.member<float>("a");

	world.component<Sprite>()
		.member<glm::vec2>("texture_size")
		.member<glm::vec2>("origin")
		.member<std::optional<glm::vec2>>("custom_size")
		.member<SDL_FColor>("color")
		.member<std::string>("path");

	world.observer<Sprite>()
		.event(flecs::OnSet)
		.each([](flecs::entity e, Sprite& s) {
			e.add<Dirty>();
		});

	world.system<Application, RenderDevice, SpritePipeline>("create sprite pipeline")
		.kind(Phases::OnStart)
		.each([](Application& app, RenderDevice& device, SpritePipeline& pipeline) {
			auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/sprite.vert.msl", 1);
			auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/sprite.frag.msl", 0, 1);

			std::array<SDL_GPUVertexAttribute, 3> vertex_attrs{
				SDL_GPUVertexAttribute{
					.location = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = offsetof(internal::VertexData, position),
				},
				SDL_GPUVertexAttribute{
					.location = 1,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = offsetof(internal::VertexData, color),
				},
				SDL_GPUVertexAttribute{
					.location = 2,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
					.offset = offsetof(internal::VertexData, uv),
				},
			};

			SDL_GPUVertexBufferDescription vertex_buffer_description{
				.slot = 0,
				.pitch = sizeof(internal::VertexData),
			};

			SDL_GPUColorTargetDescription color_target_description{
				.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, app.window)
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

			pipeline.sampler = SDL_CreateGPUSampler(device.gpu, &sampler_create_info);
			pipeline.pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

			SDL_ReleaseGPUShader(device.gpu, vert_shader);
			SDL_ReleaseGPUShader(device.gpu, frag_shader);
		});

	world.system<Sprite, GlobalTransform, RenderPass, SpritePipeline, RenderDevice, QuadMesh>()
		.kind(Phases::Render)
		.each([world](Sprite& sprite, GlobalTransform& transform, RenderPass& render_pass, SpritePipeline& pipeline, RenderDevice& device, QuadMesh& quad) {
			const auto camera = world.entity(CameraModule::EcsCamera);
			const auto view = glm::translate(glm::mat4(1.f), -camera.get<GlobalTransform>().translation);
			const auto projection = camera.get<Camera>().projection;

			internal::UBO ubo{ 
				.mvp = projection * view * (transform.matrix * glm::scale(glm::mat4(1.f), glm::vec3(sprite.texture->get_size(), 0.f)))
			};

			SDL_GPUBufferBinding vertex_buffer_binding{
				.buffer = quad.vertex_buffer,
			};
			SDL_GPUBufferBinding index_buffer_binding{
				.buffer = quad.index_buffer,
			};

			SDL_BindGPUGraphicsPipeline(render_pass.render_pass, pipeline.pipeline);
			SDL_BindGPUVertexBuffers(render_pass.render_pass, 0, &vertex_buffer_binding, 1);
			SDL_BindGPUIndexBuffer(render_pass.render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
			SDL_PushGPUVertexUniformData(render_pass.cmd_buffer, 0, &ubo, sizeof(ubo));

			SDL_GPUTextureSamplerBinding texture_sampler_binding{
				.texture = &sprite.texture->get_gpu_texture(),
				.sampler = pipeline.sampler,
			};

			SDL_BindGPUFragmentSamplers(render_pass.render_pass, 0, &texture_sampler_binding, 1);

			SDL_DrawGPUIndexedPrimitives(render_pass.render_pass, 6, 1, 0, 0, 0);
		});

	//world.system<Application, Renderer, SpriteRenderer>("create sprite pipeline")
	//	.kind(Phases::OnStart)
	//	.each([](Application& app, Renderer& r, SpriteRenderer& sp) {
	//		auto vert_shader = load_shader(*r.gpu, "assets/shaders/out/shader.vert.msl", 1, 0, 1);
	//		auto frag_shader = load_shader(*r.gpu, "assets/shaders/out/shader.frag.msl", 0, 1);

	//		SDL_GPUColorTargetDescription color_target_description{
	//			.format = SDL_GetGPUSwapchainTextureFormat(r.gpu, app.window),
	//			.blend_state = {
	//				.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	//				.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	//				.color_blend_op = SDL_GPU_BLENDOP_ADD,
	//				.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	//				.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	//				.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
	//				.enable_blend = true,
	//			}
	//		};

	//		SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{
	//			.vertex_shader = vert_shader,
	//			.fragment_shader = frag_shader,
	//			.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
	//			.target_info = {
	//				.color_target_descriptions = &color_target_description,
	//				.num_color_targets = 1,
	//			}
	//		};

	//		auto pipeline = SDL_CreateGPUGraphicsPipeline(r.gpu, &pipeline_create_info);

	//		SDL_ReleaseGPUShader(r.gpu, vert_shader);
	//		SDL_ReleaseGPUShader(r.gpu, frag_shader);

	//		if (!pipeline) {
	//			print_sdl_error();

	//			return;
	//		}

	//		sp.pipeline = pipeline;

	//		SDL_GPUSamplerCreateInfo sampler_create_info{};
	//		sp.sampler = SDL_CreateGPUSampler(r.gpu, &sampler_create_info);

	//		SDL_GPUBufferCreateInfo buffer_create_info{
	//			.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ,
	//			.size = sizeof(SpriteData),
	//		};
	//		sp.buffer = SDL_CreateGPUBuffer(r.gpu, &buffer_create_info);

	//		SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
	//			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
	//			.size = sizeof(SpriteData),
	//		};
	//		sp.transfer_buffer = SDL_CreateGPUTransferBuffer(r.gpu, &transfer_buffer_create_info);
	//	});

	//world.system<Renderer, SpriteRenderer, AssetStorage, GlobalTransform, Mesh, Material, ModelMatrix, Sprite>()
	//	.term_at(0).singleton()
	//	.term_at(1).singleton()
	//	.term_at(2).singleton()
	//	.with<Dirty>()
	//	.kind(Phases::Update)
	//	.each([](Renderer& r, SpriteRenderer& sp, AssetStorage& as, GlobalTransform& t, Mesh& m, Material& mat, ModelMatrix& mm, Sprite& s) {
	//		mat.pipeline = sp.pipeline;
	//		mat.sampler = sp.sampler;
	//		if (!s.path.empty()) {
	//			mat.texture = as.load_texture(*r.gpu, s.path);
	//			if (mat.texture) {
	//				s.texture_size = mat.texture->get_size();
	//			}
	//		} else {
	//			mat.texture = std::make_shared<Texture>(r.gpu, r.white_texture, s.custom_size ? s.custom_size.value() : s.texture_size);
	//		}
	//		mm.matrix = t.matrix * glm::scale(glm::mat4(1.f), glm::vec3(s.custom_size ? s.custom_size.value() : s.texture_size, 0.f));
	//	});

	//world.system<GlobalTransform, Sprite>()
	//	.kind(Phases::Render)
	//	.run([](flecs::iter& it) {
	//		const auto& app = it.world().get<Application>();
	//		const auto& renderer = it.world().get<Renderer>();
	//		const auto& sprite_renderer = it.world().get<SpriteRenderer>();
	//		const auto camera = it.world().entity(CameraModule::EcsCamera);
	//		const auto view = glm::translate(glm::mat4(1.f), -camera.get<GlobalTransform>().translation);
	//		const auto& projection = camera.get<Camera>().projection;

	//		auto cmd_buf = SDL_AcquireGPUCommandBuffer(renderer.gpu);

	//		SDL_GPUTexture* swapchain_texture;

	//		auto result = SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, app.window, &swapchain_texture, nullptr, nullptr); assert(result);

	//		auto color_target = SDL_GPUColorTargetInfo{
	//			.texture = swapchain_texture,
	//			.clear_color = { 0.f, 0.f, 0.f, 1.f },
	//			.load_op = SDL_GPU_LOADOP_CLEAR,
	//			.store_op = SDL_GPU_STOREOP_STORE
	//		};

	//		internal::UBO ubo{ 
	//			.view_proj = projection * view
	//		};

	//		auto data = static_cast<SpriteData*>(SDL_MapGPUTransferBuffer(
	//			renderer.gpu,
	//			sprite_renderer.transfer_buffer,
	//			true
	//		));
	//		auto data_i = 0;

	//		while(it.next()) {
	//			auto transforms = it.field<GlobalTransform>(0);
	//			auto sprites = it.field<Sprite>(1);

	//			for (auto i : it) {
	//				data[data_i].position = transforms[i].translation;
	//				data[data_i].scale    = {0.5f, 0.5f }; //transforms[i].scale;
	//				data[data_i].rotation = transforms[i].rotation.z;
	//				data[data_i].scale    = glm::vec2{ 2560.f, 1440.f };
	//				data[data_i].u        = 0.f;
	//				data[data_i].v        = 0.f;
	//				data[data_i].w        = 1.f;
	//				data[data_i].h        = 1.f;
	//				data[data_i].color    = sprites[i].color;

	//				++data_i;
	//			}
	//		}

	//		SDL_UnmapGPUTransferBuffer(renderer.gpu, sprite_renderer.transfer_buffer);

	//		// Upload instance data
	//		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);

	//		SDL_GPUTransferBufferLocation transfer_buffer_location{
	//			.transfer_buffer = sprite_renderer.transfer_buffer,
	//			.offset = 0
	//		};
	//		SDL_GPUBufferRegion buffer_region{
	//			.buffer = sprite_renderer.buffer,
	//			.offset = 0,
	//			.size = sizeof(SpriteData),
	//		};

	//		SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, true);

	//		SDL_EndGPUCopyPass(copy_pass);

	//		// Render sprites
	//		SDL_GPUColorTargetInfo color_target_info{
	//			.texture = swapchain_texture,
	//			.clear_color = { 0, 0, 0, 1 },
	//			.load_op = SDL_GPU_LOADOP_CLEAR,
	//			.store_op = SDL_GPU_STOREOP_STORE,
	//			.cycle = false,
	//		};

	//		SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target_info, 1, nullptr);

	//		SDL_BindGPUGraphicsPipeline(render_pass, sprite_renderer.pipeline);
	//		SDL_BindGPUVertexStorageBuffers(render_pass, 0, &sprite_renderer.buffer, 1);

	//		SDL_GPUTextureSamplerBinding texture_binding{
	//			.texture = &sprite_renderer.texture->get_gpu_texture(),
	//			.sampler = sprite_renderer.sampler
	//		};
	//		SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);
	//		SDL_PushGPUVertexUniformData(cmd_buf, 0, &ubo, sizeof(ubo));
	//		SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);

	//		SDL_EndGPURenderPass(render_pass);

	//		result &= SDL_SubmitGPUCommandBuffer(cmd_buf); assert(result);
	//	});

	world.system<AssetStorage, RenderDevice>()
		.kind(Phases::OnStart)
		.each([](flecs::iter& it, size_t, AssetStorage& assets, RenderDevice& device) {
			it.world().entity("sprite")
				.set<Sprite>({ .texture = assets.load_texture(*device.gpu, "assets/main_menu.jpg") });
			it.world().entity("sprite2")
				.set<Sprite>({ .texture = assets.load_texture(*device.gpu, "assets/red.png") })
				.set<Transform>({ .translation = {50.f, 50.f, 0.f } });
		});

	world.add<SpritePipeline>();
}
