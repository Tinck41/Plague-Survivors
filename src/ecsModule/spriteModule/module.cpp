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

void draw_sprite(flecs::entity_t entity, const flecs::world& world) {
	const auto& batches = world.get<SpriteBatches>();

	if (!batches.batches.contains(entity)) {
		return;
	}

	const auto& pipeline = world.get<SpritePipeline>();
	const auto& storage = world.get<SpriteStorage>();
	const auto& render_commands = world.get<RenderCommands>();

	const auto camera = world.entity(CameraModule::EcsCamera);
	const auto view = glm::translate(glm::mat4(1.f), -camera.get<GlobalTransform>().translation);
	const auto view_proj = view * camera.get<Camera>().projection;

	const auto& batch = batches.batches.at(entity);

	SDL_GPUBufferBinding vertex_buffer_binding{
		.buffer = storage.instance_buffer,
	};
	SDL_GPUBufferBinding index_buffer_binding{
		.buffer = storage.index_buffer,
	};

	SDL_GPUTextureSamplerBinding texture_sampler_binding{
		.texture = &batch.texture->get_gpu_texture(),
		.sampler = pipeline.sampler,
	};

	SDL_BindGPUGraphicsPipeline(render_commands.render_pass, pipeline.pipeline);
	SDL_BindGPUVertexBuffers(render_commands.render_pass, 0, &vertex_buffer_binding, 1);
	SDL_BindGPUIndexBuffer(render_commands.render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
	SDL_BindGPUFragmentSamplers(render_commands.render_pass, 0, &texture_sampler_binding, 1);

	SDL_PushGPUVertexUniformData(render_commands.cmd_buffer, 0, &view_proj, sizeof(glm::mat4));

	SDL_DrawGPUIndexedPrimitives(render_commands.render_pass, 6, batch.size, 0, 0, batch.first_instance);
}

SpriteModule::SpriteModule(flecs::world& world) {
	world.module<SpriteModule>();

	world.import<AppModule>();
	world.import<RenderModule>();
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
		.member<glm::vec2>("origin")
		.member<std::optional<glm::vec2>>("custom_size")
		.member<SDL_FColor>("color")
		.member<std::string>("path");

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

			SDL_GPUTextureCreateInfo texture_create_info{
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
				.width = 1,
				.height = 1,
				.layer_count_or_depth = 1,
				.num_levels = 1,
			};

			auto texture = SDL_CreateGPUTexture(device.gpu, &texture_create_info);

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = 4,
			};

			auto tex_transfer_buf = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);
			auto tex_transfer_mem = SDL_MapGPUTransferBuffer(device.gpu, tex_transfer_buf, false);

			uint32_t white_pixel = 0xffffffff;
			std::memcpy(tex_transfer_mem, &white_pixel, 4);

			SDL_UnmapGPUTransferBuffer(device.gpu, tex_transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(device.gpu);
			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

			SDL_GPUTextureTransferInfo texture_transfer_info{
				.transfer_buffer = tex_transfer_buf,
			};
			SDL_GPUTextureRegion texture_region{
				.texture = texture,
				.w = 1,
				.h = 1,
				.d = 1,
			};

			SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);

			SDL_EndGPUCopyPass(copy_pass);

			assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());

			SDL_ReleaseGPUTransferBuffer(device.gpu, tex_transfer_buf);

			SDL_GPUSamplerCreateInfo sampler_create_info{};

			pipeline.sampler = SDL_CreateGPUSampler(device.gpu, &sampler_create_info);
			pipeline.pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);
			pipeline.white_texture = std::make_shared<Texture>(device.gpu, texture, glm::vec2(1.f, 1.f));

			SDL_ReleaseGPUShader(device.gpu, vert_shader);
			SDL_ReleaseGPUShader(device.gpu, frag_shader);
		});

	world.system<RenderDevice, CopyCommands, SpriteStorage>()
		.kind(Phases::OnStart)
		.each([](RenderDevice& device, CopyCommands& commands, SpriteStorage& storage) {
			std::array<uint16_t, 6> indecies{
				0, 1, 2,
				2, 1, 3
			};

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = max_sprites_per_batch * sizeof(SpriteInstance),
			};

			storage.transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

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

			// TODO: use storage transfer buf?
			SDL_GPUTransferBufferCreateInfo indecies_transfer_buffer_create_info{
				.usage = SDL_GPUTransferBufferUsage::SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<uint32_t>(indecies_byte_szie)
			};

			auto transfer_buf = SDL_CreateGPUTransferBuffer(device.gpu, &indecies_transfer_buffer_create_info);

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

			batches.batches.clear();

			auto instances = static_cast<SpriteInstance*>(SDL_MapGPUTransferBuffer(device.gpu, storage.transfer_buffer, false));

			uint32_t current_instance = 0;

			flecs::entity_t current_batch_entity;

			while (it.next()) {
				auto sprites = it.field<Sprite>(0);
				auto transforms = it.field<GlobalTransform>(1);

				for (auto i : it) {
					auto& sprite = sprites[i];
					auto& transform = transforms[i];

					if (!sprite.texture) {
						sprite.texture = pipeline.white_texture;
					}

					if (batches.batches.empty() || batches.batches.at(current_batch_entity).texture != sprite.texture || batches.batches.at(current_batch_entity).size >= max_sprites_per_batch) {
						current_batch_entity = it.entity(i);

						batches.batches.emplace(current_batch_entity, SpriteBatch{
							.size = 0,
							.first_instance = current_instance,
							.texture = sprite.texture,
						});
					}

					auto& current_batch = batches.batches.at(current_batch_entity);

					instances[current_instance].translation = glm::vec4(transform.translation, 0.f);
					instances[current_instance].rotation    = glm::vec4(transform.rotation, 0.f);
					instances[current_instance].scale       = glm::vec4(transform.scale * glm::vec3(sprite.custom_size.value_or(sprite.texture->get_size()), 0.f), 0.f);
					instances[current_instance].color       = sprite.color;
					instances[current_instance].uv          = glm::vec2(0.f, 0.f); // TODO
					instances[current_instance].size        = glm::vec2(1.f, 1.f); // TODO

					++current_batch.size;
					++current_instance;
				}
			}

			SDL_UnmapGPUTransferBuffer(device.gpu, storage.transfer_buffer);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_commands.buffer);

			for (const auto& [_, batch] : batches.batches) {
				const auto offset = static_cast<Uint32>(batch.first_instance * sizeof(SpriteInstance));

				const auto buffer_location = SDL_GPUTransferBufferLocation{
					.transfer_buffer = storage.transfer_buffer,
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

	world.system<const Sprite, const GlobalTransform, const SpritePipeline, RenderItems>()
		.kind(Phases::PostUpdate)
		.each([](flecs::entity e, const Sprite& sprite, const GlobalTransform& transform, const SpritePipeline& pipeline, RenderItems& items) {
			items.emplace_back(e, transform.translation.z, &draw_sprite);
		});

	world.system<AssetStorage, RenderDevice>()
		.kind(Phases::OnStart)
		.each([](flecs::iter& it, size_t, AssetStorage& assets, RenderDevice& device) {
			it.world().entity("sprite")
				.set<Sprite>({ .texture = assets.load_texture(*device.gpu, "assets/COUPON.png") });
			it.world().entity("sprite2")
				.set<Sprite>({ .custom_size = glm::vec2{ 100.f, 100.f }, .color = { 1.f, 0.f, 0.f, 1.f } });
		});

	world.add<SpritePipeline>();
	world.add<SpriteBatches>();
	world.add<SpriteStorage>();
}
