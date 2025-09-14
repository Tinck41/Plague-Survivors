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

constexpr uint32_t max_sprites_per_batch = 10'000;

SpriteModule::SpriteModule(flecs::world& world) {
	world.module<SpriteModule>();

	world.import<AppModule>();
	world.import<RenderModule>();
	world.import<MeshModule>();
	world.import<TransformModule>();

	world.component<SpritePipeline>().add(flecs::Singleton);
	world.component<SpriteBatches>().add(flecs::Singleton);
	world.component<SpriteStorage>().add(flecs::Singleton);
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
			auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/sprite_batch.vert.msl", 1);
			auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/sprite_batch.frag.msl", 0, 1);

			SDL_GPUColorTargetDescription color_target_description{
				.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, app.window),
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

			pipeline.sampler = SDL_CreateGPUSampler(device.gpu, &sampler_create_info);
			pipeline.pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

			SDL_ReleaseGPUShader(device.gpu, vert_shader);
			SDL_ReleaseGPUShader(device.gpu, frag_shader);
		});

	world.system<RenderDevice, SpriteBatches>()
		.kind(Phases::OnStart)
		.each([](RenderDevice& device, SpriteBatches& batches) {
			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = max_sprites_per_batch * sizeof(SpriteInstance),
			};

			const auto buffer_create_info = SDL_GPUBufferCreateInfo{
				.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
				.size = max_sprites_per_batch * sizeof(SpriteInstance),
			};

			batches.transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);
		});

	world.system<RenderDevice, CopyCommands, SpriteStorage>()
		.kind(Phases::OnStart)
		.each([](RenderDevice& device, CopyCommands& commands, SpriteStorage& storage) {
			constexpr auto white = glm::vec4{ 1, 1, 1, 1 };

			std::array<uint16_t, 6> indecies{
				0, 1, 2,
				2, 3, 0
			};

			const auto instances_byte_szie = 100'000 * sizeof(SpriteInstance);
			const auto indecies_byte_szie = indecies.size() * sizeof(indecies[0]);

			SDL_GPUBufferCreateInfo instance_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = instances_byte_szie,
			};
			SDL_GPUBufferCreateInfo index_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_INDEX,
				.size = indecies_byte_szie,
			};

			storage.instance_buffer = SDL_CreateGPUBuffer(device.gpu, &instance_buffer_create_info);
			storage.index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPUTransferBufferUsage::SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<uint32_t>(indecies_byte_szie)
			};

			auto transfer_buf = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

			auto transfer_mem = SDL_MapGPUTransferBuffer(device.gpu ,transfer_buf, false);

			std::memcpy(transfer_mem, indecies.data(), indecies_byte_szie);

			SDL_UnmapGPUTransferBuffer(device.gpu, transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(device.gpu);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

			SDL_GPUTransferBufferLocation index_buffer_location{ .transfer_buffer = transfer_buf };

			SDL_GPUBufferRegion index_buffer_region{ .buffer = storage.index_buffer, .size = static_cast<uint32_t>(indecies_byte_szie) };

			SDL_UploadToGPUBuffer(copy_pass, &index_buffer_location, &index_buffer_region, false);

			SDL_EndGPUCopyPass(copy_pass);

			assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());
		});

	world.system<Sprite, GlobalTransform>()
		.kind(Phases::PostUpdate)
		.run([](flecs::iter& it) {
			const auto world = it.world();

			const auto& copy_commands = it.world().get<CopyCommands>();
			const auto& pipeline = it.world().get<SpritePipeline>();
			const auto& device = it.world().get<RenderDevice>();
			const auto& storage = it.world().get<SpriteStorage>();

			auto& batches = it.world().get_mut<SpriteBatches>();

			auto instances = static_cast<SpriteInstance*>(SDL_MapGPUTransferBuffer(device.gpu, batches.transfer_buffer, false));

			uint32_t current_instance = 0;

			while (it.next()) {
				auto sprites = it.field<Sprite>(0);
				auto transforms = it.field<GlobalTransform>(1);

				for (auto i : it) {
					auto& sprite = sprites[i];
					auto& transform = transforms[i];

					if (batches.batches.empty() || batches.batches.back().texture != sprite.texture || batches.batches.back().size >= max_sprites_per_batch) {
						batches.batches.emplace_back(SpriteBatch{
							.size = 0,
							.first_instance = current_instance,
							.texture = sprite.texture,
						});
					}

					auto& current_batch = batches.batches.back();

					instances[current_instance].translation = glm::vec4(transform.translation, 0.f);
					instances[current_instance].rotation    = glm::vec4(transform.rotation, 0.f);
					instances[current_instance].scale       = glm::vec4(transform.scale * glm::vec3(sprite.texture->get_size(), 0.f), 0.f);
					instances[current_instance].color       = sprite.color;
					instances[current_instance].uv          = glm::vec2(0.f, 0.f); // TODO
					instances[current_instance].size        = glm::vec2(1.f, 1.f); // TODO

					++current_batch.size;
					++current_instance;
				}
			}

			SDL_UnmapGPUTransferBuffer(device.gpu, batches.transfer_buffer);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_commands.buffer);

			for (const auto& batch : batches.batches) {
				const auto offset = static_cast<Uint32>(batch.first_instance * sizeof(SpriteInstance));

				const auto buffer_location = SDL_GPUTransferBufferLocation{
					.transfer_buffer = batches.transfer_buffer,
					.offset = offset,
				};

				const auto buffer_region = SDL_GPUBufferRegion{
					.buffer = storage.instance_buffer,
					.offset = offset,
					.size = static_cast<Uint32>(batch.size * sizeof(SpriteInstance))
				};

				SDL_UploadToGPUBuffer(copy_pass,  &buffer_location, &buffer_region, false);
			}

			SDL_EndGPUCopyPass(copy_pass);
		});

	world.system<SpriteBatches, RenderCommands, SpritePipeline, SpriteStorage>()
		.kind(Phases::Render)
		.each([world](SpriteBatches& batches, RenderCommands& render_commands, SpritePipeline& pipeline, SpriteStorage& storage) {
			const auto camera = world.entity(CameraModule::EcsCamera);
			const auto view = glm::translate(glm::mat4(1.f), -camera.get<GlobalTransform>().translation);
			const auto projection = camera.get<Camera>().projection;
			const auto view_proj = view * projection;

			SDL_GPUBufferBinding vertex_buffer_binding{
				.buffer = storage.instance_buffer,
			};
			SDL_GPUBufferBinding index_buffer_binding{
				.buffer = storage.index_buffer,
			};

			SDL_BindGPUGraphicsPipeline(render_commands.render_pass, pipeline.pipeline);
			SDL_BindGPUVertexBuffers(render_commands.render_pass, 0, &vertex_buffer_binding, 1);
			SDL_BindGPUIndexBuffer(render_commands.render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
			//SDL_BindGPUVertexStorageBuffers(render_commands.render_pass, 0, &batches.data_buffer, 1);

			SDL_PushGPUVertexUniformData(render_commands.cmd_buffer, 0, &view_proj, sizeof(glm::mat4));

			auto draw_calls_count = 0;

			for (const auto& batch : batches.batches) {
				SDL_GPUTextureSamplerBinding texture_sampler_binding{
					.texture = &batch.texture->get_gpu_texture(),
					.sampler = pipeline.sampler,
				};

				SDL_BindGPUFragmentSamplers(render_commands.render_pass, 0, &texture_sampler_binding, 1);
				SDL_DrawGPUIndexedPrimitives(render_commands.render_pass, 6, batch.size, 0, 0, batch.first_instance);

				++draw_calls_count;
			}

			batches.batches.clear();

			spdlog::info("draw calls: {}", draw_calls_count);
		});

	world.system<AssetStorage, RenderDevice>()
		.kind(Phases::OnStart)
		.each([](flecs::iter& it, size_t, AssetStorage& assets, RenderDevice& device) {
			it.world().entity("sprite")
				.set<Sprite>({ .texture = assets.load_texture(*device.gpu, "assets/main_menu.jpg") });
			it.world().entity("sprite2")
				.set<Sprite>({ .texture = assets.load_texture(*device.gpu, "assets/red.png") })
				.set<Transform>({ .translation = {150.f, 150.f, 0.f } });
			it.world().entity("sprite3")
				.set<Sprite>({ .texture = assets.load_texture(*device.gpu, "assets/main_menu.jpg") })
				.set<Transform>({ .translation = {200.f, 200.f, 0.f } });
			it.world().entity("sprite4")
				.set<Sprite>({ .texture = assets.load_texture(*device.gpu, "assets/main_menu.jpg") })
				.set<Transform>({ .translation = {300.f, 200.f, 0.f } });
			it.world().entity("sprite5")
				.set<Sprite>({ .texture = assets.load_texture(*device.gpu, "assets/red.png") })
				.set<Transform>({ .translation = {450.f, 150.f, 0.f } });
		});

	world.add<SpritePipeline>();
	world.add<SpriteBatches>();
	world.add<SpriteStorage>();
}
