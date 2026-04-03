#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/windowModule/module.h"
#include "ext/matrix_transform.hpp"
#include "utils/sdl.h"

#include <ranges>

using namespace ps;

constexpr uint32_t max_sprites_per_batch = 10'000;

void draw_sprite(flecs::entity_t camera_entity, flecs::entity_t entity, const flecs::world& world) {
	const auto& batches = world.get<CameraSpriteBatches>();

	if (!batches.contains(camera_entity) || !batches.at(camera_entity).contains(entity)) {
		return;
	}

	const auto& pipeline = world.get<SpritePipeline>();
	const auto& storage = world.get<SpriteStorage>();
	const auto& render_commands = world.get<RenderCommands>();

	const auto camera = world.entity(camera_entity);
	const auto view = glm::translate(glm::mat4(1.f), -camera.get<GlobalTransform>().translation);
	const auto view_proj = camera.get<Camera>().projection * view;

	const auto& render_pass = camera.get<RenderPass>().render_pass;
	const auto& batch = batches.at(camera_entity).at(entity);

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

	SDL_BindGPUGraphicsPipeline(render_pass, pipeline.pipeline);
	SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
	SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
	SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);

	SDL_PushGPUVertexUniformData(render_commands.cmd_buffer, 0, &view_proj, sizeof(glm::mat4));

	SDL_DrawGPUIndexedPrimitives(render_pass, 6, batch.size, 0, 0, batch.first_instance);
}

SpriteModule::SpriteModule(flecs::world& world) {
	world.module<SpriteModule>();

	world.import<RenderModule>();
	world.import<TransformModule>();
	world.import<CameraModule>();

	world.component<SpritePipeline>().add(flecs::Singleton);
	world.component<SpriteStorage>().add(flecs::Singleton);
	world.component<CameraSpriteBatches>().add(flecs::Singleton);
	world.component<CameraCollectedSpriteItems>().add(flecs::Singleton);
	world.component<Sprite>()
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<RenderLayers>())
		.add(flecs::With, world.component<Visible2d>())
		.add(flecs::With, world.component<Aabb>());

	world.component<Sprite>()
		.member<glm::vec2>("origin")
		.member<Color>("color")
		.member<std::optional<TextureAtlas>>("texture_atlas")
		.member<std::optional<glm::vec2>>("custom_size");

	auto transparend_2d = world.component<Transparent2d>()
		.is_a<RenderPhase>();

	RenderModule::render_phases_order.emplace_back(transparend_2d);

	world.observer<Sprite, WhiteTexture>()
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Sprite& sprite, WhiteTexture& white_texture) {
			sprite.texture = white_texture.texture;
		});

	world.system<RenderDevice, SpritePipeline>("create sprite pipeline")
		.kind(Phases::OnStart)
		.each([&world](RenderDevice& device, SpritePipeline& pipeline) {
			auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/sprite_batch.vert.msl", 1);
			auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/sprite_batch.frag.msl", 0, 1);

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

			pipeline.sampler = SDL_CreateGPUSampler(device.gpu, &sampler_create_info);
			pipeline.pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

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

	world.system<Sprite, Aabb, GlobalTransform>()
		.kind(Phases::Update)
		.each([](Sprite& sprite, Aabb& aabb, GlobalTransform& transform) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + sprite.texture->get_size();
		});

	auto sprite_query = world.query<Sprite, GlobalTransform, Visible2d>();

	world.system<VisibleEntities, CameraRenderPhaseItems, CameraCollectedSpriteItems>("collect sprites")
		.with<Camera>()
		.kind(Phases::CollectRenderData)
		.each([transparend_2d, sprite_query](flecs::entity camera_entity, VisibleEntities& visible_entities, CameraRenderPhaseItems& render_items, CameraCollectedSpriteItems& sprite_items) {
			sprite_query.each([&](flecs::entity entity, Sprite& sprite, GlobalTransform& transform, Visible2d& visible) {
				if (!visible.value) {
					return;
				}

				if (!visible_entities.entities.contains(entity)) {
					return;
				}

				const auto [size, uv] = [&sprite] {
					//if (sprite.texture_atlas) {
					//	const auto& rect = sprite.texture_atlas.value().textures.at(sprite.texture_atlas.value().current_index);
					//	const auto size = glm::vec2(rect.w, rect.h);
					//	const auto uv = glm::vec2(rect.x, rect.y) / sprite.texture->get_size();

					//	return std::make_pair(size, uv);
					//}

					return std::make_pair(sprite.texture->get_size(), glm::vec2(1.f, 1.f));
				}();

				render_items[camera_entity][transparend_2d].emplace_back(entity, &draw_sprite, transform.translation.z);

				sprite_items[camera_entity].lookup[entity] = sprite_items[camera_entity].items.size();
				sprite_items[camera_entity].items.emplace_back(
					entity,
					sprite.texture,
					transform,
					sprite.color,
					size,
					uv,
					SpriteSingle{
						.custom_size = sprite.custom_size
					}
				);
			});
		});

	world.system<CameraCollectedSpriteItems, CameraRenderPhaseItems, CameraSpriteBatches, SpriteStorage, RenderDevice, CopyCommands>("generate sprite batches")
		.kind(Phases::PrepareRenderData)
		.each([transparend_2d](CameraCollectedSpriteItems& camera_sprite_items, CameraRenderPhaseItems& camera_render_items, CameraSpriteBatches& camera_batches, SpriteStorage& storage, RenderDevice& device, CopyCommands& copy_commands) {
			camera_batches.clear();

			auto instances = static_cast<SpriteInstance*>(SDL_MapGPUTransferBuffer(device.gpu, storage.transfer_buffer, false));
			uint32_t current_instance = 0;

			for (auto& [camera, camera_phase_items] : camera_render_items) {
				auto& phase_items = camera_phase_items[transparend_2d];
				auto& sprite_items = camera_sprite_items[camera];
				auto& batches = camera_batches[camera];

				flecs::entity_t current_batch_entity = flecs::entity::null();
				size_t current_batch_index = 0;

				for (size_t i = 0; i < phase_items.size(); ++i) {
					const auto& render_data = phase_items[i];

					if (!sprite_items.lookup.contains(render_data.entity)) {
						current_batch_entity = flecs::entity::null();

						continue;
					}

					const auto& sprite = sprite_items.items[sprite_items.lookup[render_data.entity]];

					if (!current_batch_entity || batches.at(current_batch_entity).texture != sprite.texture || batches.at(current_batch_entity).size >= max_sprites_per_batch) {
						current_batch_entity = sprite.entity;
						current_batch_index = i;

						batches.emplace(current_batch_entity, SpriteBatch{
							.size = 0,
							.first_instance = current_instance,
							.texture = sprite.texture,
						});
					}

					auto& current_batch = batches.at(current_batch_entity);

					instances[current_instance].translation = glm::vec4(sprite.transform.translation, 0.f);
					instances[current_instance].rotation    = glm::vec4(sprite.transform.rotation, 0.f);
					instances[current_instance].scale       = glm::vec4(sprite.transform.scale * glm::vec3(sprite.size, 0.f), 0.f);
					instances[current_instance].color       = sprite.color;
					instances[current_instance].uv          = sprite.uv;
					instances[current_instance].size        = sprite.size / sprite.texture->get_size();

					++current_batch.size;
					++current_instance;
					++phase_items[current_batch_index].batch_size;
				}

				sprite_items.items.clear();
				sprite_items.lookup.clear();
			}

			if (current_instance == 0) {
				return;
			}

			SDL_UnmapGPUTransferBuffer(device.gpu, storage.transfer_buffer);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_commands.buffer);

			const auto buffer_location = SDL_GPUTransferBufferLocation{
				.transfer_buffer = storage.transfer_buffer,
			};

			const auto buffer_region = SDL_GPUBufferRegion{
				.buffer = storage.instance_buffer,
				.size = static_cast<Uint32>(current_instance * sizeof(SpriteInstance))
			};

			SDL_UploadToGPUBuffer(copy_pass,  &buffer_location, &buffer_region, false);

			SDL_EndGPUCopyPass(copy_pass);
		});

	world.add<SpritePipeline>();
	world.add<SpriteStorage>();
	world.add<CameraSpriteBatches>();
	world.add<CameraCollectedSpriteItems>();
}
