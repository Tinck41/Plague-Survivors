#include "module.h"
#include "components.h"
#include "ecsModule/common.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/windowModule/components.h"
#include "ecsModule/windowModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/meshModule/module.h"
#include "ecsModule/textModule/module.h"
#include "ext/matrix_clip_space.hpp"
#include "utils/sdl.h"
#include "utils/visit.h"
#include "font.h"

#include <ranges>

using namespace ps;

constexpr Uint32 max_ui_per_batch = 10'000;

#define UI_TEXT_MAX_VERTEX_COUNT 4000
#define UI_TEXT_MAX_INDEX_COUNT  6000

void bind_data(const flecs::world& world) {
	const auto& pipeline = world.get<UiPipeline>();
	const auto& storage = world.get<UiStorage>();
	const auto& render_commands = world.get<RenderCommands>();
	const auto& window = world.get<Window>();

	const auto proj = glm::ortho(0.f, float(window.width), float(window.height), 0.f);

	SDL_GPUBufferBinding vertex_buffer_binding{
		.buffer = storage.vertex_buffer,
	};
	SDL_GPUBufferBinding index_buffer_binding{
		.buffer = storage.index_buffer,
	};

	SDL_BindGPUGraphicsPipeline(render_commands.render_pass, pipeline.pipeline);
	SDL_BindGPUVertexBuffers(render_commands.render_pass, 0, &vertex_buffer_binding, 1);
	SDL_BindGPUIndexBuffer(render_commands.render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	SDL_PushGPUVertexUniformData(render_commands.cmd_buffer, 0, &proj, sizeof(glm::mat4));
}

void bind_texture(flecs::entity_t entity, const flecs::world& world) {
	const auto& batches = world.get<UiBatches>();

	if (!batches.contains(entity)) {
		return;
	}

	const auto& pipeline = world.get<UiPipeline>();
	const auto& render_commands = world.get<RenderCommands>();

	const auto& batch = batches.at(entity);

	SDL_GPUTextureSamplerBinding texture_sampler_binding{
		.texture = &batch.texture->get_gpu_texture(),
		.sampler = pipeline.sampler,
	};

	SDL_BindGPUFragmentSamplers(render_commands.render_pass, 0, &texture_sampler_binding, 1);
}

void draw_ui(flecs::entity_t entity, const flecs::world& world) {
	const auto& batches = world.get<UiBatches>();

	if (!batches.contains(entity)) {
		return;
	}

	const auto& batch = batches.at(entity);
	const auto& render_commands = world.get<RenderCommands>();

	SDL_DrawGPUIndexedPrimitives(render_commands.render_pass, 6 * batch.size, 1, batch.first_index, 0, 0);
}

void draw(flecs::entity_t camera, flecs::entity_t entity, const flecs::world& world) {
	const auto& batches = world.get<CameraUiBatches>();

	if (!batches.contains(camera) || !batches.at(camera).contains(entity)) {
		return;
	}

	const auto& pipeline = world.get<UiPipeline>();
	const auto& storage = world.get<UiStorage>();
	const auto& render_commands = world.get<RenderCommands>();
	const auto camera_entity = world.entity(camera);
	const auto& render_pass = camera_entity.get<RenderPass>().render_pass;
	const auto& camera_data = camera_entity.get<Camera>();

	const auto proj = visit(camera_data.render_target, visitors{
		[&world](flecs::entity_t window) {
			const auto& window_data = world.entity(window).get<Window>();

			return glm::ortho(0.f, float(window_data.width), float(window_data.height), 0.f);
		},
		[](const std::shared_ptr<Texture>& texture) {
			const auto texture_size = texture->get_size();
			return glm::ortho(0.f, texture_size.x, texture_size.y, 0.f);
		},
		[](auto&&) {
			return glm::ortho(0.f, 0.f, 0.f, 0.f);
		}
	});

	const auto& batch = batches.at(camera).at(entity);

	SDL_GPUBufferBinding vertex_buffer_binding{
		.buffer = storage.vertex_buffer,
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

	SDL_PushGPUVertexUniformData(render_commands.cmd_buffer, 0, &proj, sizeof(glm::mat4));
	SDL_PushGPUDebugGroup(render_commands.cmd_buffer, "draw_ui_node");

	SDL_DrawGPUIndexedPrimitives(render_pass, 6 * batch.size, 1, batch.first_index, 0, 0);

	SDL_PopGPUDebugGroup(render_commands.cmd_buffer);
}

void draw_ui_text(flecs::entity_t camera, flecs::entity_t entity, const flecs::world& world) {
	const auto& batches = world.get<CameraUiTextBatches>();

	if (!batches.contains(camera) || !batches.at(camera).contains(entity) || batches.at(camera).at(entity).empty()) {
		return;
	}

	const auto& pipeline = world.get<UiTextPipeline>();
	const auto& storage = world.get<UiTextStorage>();
	const auto& commands = world.get<RenderCommands>();
	const auto camera_entity = world.entity(camera);
	const auto& camera_data = camera_entity.get<Camera>();
	const auto& render_pass = camera_entity.get<RenderPass>().render_pass;

	auto& batch_seq = batches.at(camera).at(entity);

	SDL_GPUBufferBinding vertex_bindings{
		.buffer = storage.vertex_buffer,
	};
	SDL_GPUBufferBinding index_bindings{
		.buffer = storage.index_buffer,
	};

	const auto proj = visit(camera_data.render_target, visitors{
		[&world](flecs::entity_t window) {
			const auto& window_data = world.entity(window).get<Window>();

			return glm::ortho(0.f, float(window_data.width), float(window_data.height), 0.f);
		},
		[](const std::shared_ptr<Texture>& texture) {
			const auto texture_size = texture->get_size();
			return glm::ortho(0.f, texture_size.x, texture_size.y, 0.f);
		},
		[](auto&&) {
			return glm::ortho(0.f, 0.f, 0.f, 0.f);
		}
	});

	SDL_BindGPUGraphicsPipeline(render_pass, pipeline.pipeline);
	SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_bindings, 1);
	SDL_BindGPUIndexBuffer(render_pass, &index_bindings, SDL_GPU_INDEXELEMENTSIZE_32BIT);
	SDL_PushGPUVertexUniformData(commands.cmd_buffer, 0, &proj, sizeof(glm::mat4));

	for (const auto& batch : batch_seq) {
		SDL_GPUTextureSamplerBinding bindings{
			.texture = batch.texture,
			.sampler = pipeline.sampler,
		};

		SDL_BindGPUFragmentSamplers(render_pass, 0, &bindings, 1);

		SDL_DrawGPUIndexedPrimitives(render_pass, batch.num_indices, 1, batch.index_offset, 0, 0);
	}
}

UiRenderModule::UiRenderModule(flecs::world& world) {
	world.module<UiRenderModule>();

	world.import<UiModule>();
	world.import<TransformModule>();
	world.import<RenderModule>();
	world.import<CameraModule>();
	world.import<MeshModule>();
	world.import<TextModule>();

	world.component<UiPipeline>() .add(flecs::Singleton);
	world.component<UiStorage>() .add(flecs::Singleton);
	world.component<CameraUiBatches>() .add(flecs::Singleton);
	world.component<CameraCollectedUiItems>() .add(flecs::Singleton);

	world.component<UiTextStorage>().add(flecs::Singleton);
	world.component<UiTextPipeline>().add(flecs::Singleton);
	world.component<CameraCollectedUiTextItems>().add(flecs::Singleton);
	world.component<CameraUiTextBatches>().add(flecs::Singleton);

	auto transparent_ui = world.component<TransparentUi>()
		.is_a<RenderPhase>();

	transparent_ui.set<BindData>({ .function = &bind_data });
	transparent_ui.set<BindTexture>({ .function = &bind_texture });

	RenderModule::render_phases_order.emplace_back(transparent_ui);

	world.observer<Text, TextData, TextFont, UiTextPipeline>()
		.event(flecs::OnSet)
		.each([](flecs::entity e, Text& text, TextData& data, TextFont& font, UiTextPipeline& pipeline){
			if (!data.ttf_data) {
				data.ttf_data = TTF_CreateText(pipeline.engine, *font.handle, text.c_str(), text.size());
			}
			else {
				TTF_SetTextString(data.ttf_data, text.c_str(), text.size());
			}

			TTF_GetTextSize(data.ttf_data, &data.size.x, &data.size.y);

			data.size = glm::vec2(data.size) * (font.size / font.handle->get_size());
		});

	world.observer<Text, TextData>()
		.event(flecs::OnRemove)
		.each([](flecs::entity e, Text& text, TextData& data){
			TTF_DestroyText(data.ttf_data);
			data.size = { 0, 0 };
		});


	world.system<RenderDevice, UiPipeline>("create ui pipeline")
		.kind(Phases::OnStart)
		.each([&world](RenderDevice& device, UiPipeline& pipeline) {
			auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/ui.vert.msl", 1);
			auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/ui.frag.msl", 0, 1);

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

			std::array<SDL_GPUVertexAttribute, 3> vertex_attrs{
				SDL_GPUVertexAttribute{
					.location = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = offsetof(Vertex, position),
				},
				SDL_GPUVertexAttribute{
					.location = 1,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = offsetof(Vertex, color),
				},
				SDL_GPUVertexAttribute{
					.location = 2,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
					.offset = offsetof(Vertex, uv),
				},
			};

			SDL_GPUVertexBufferDescription vertex_buffer_description{
				.slot = 0,
				.pitch = sizeof(Vertex),
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

			SDL_GPUSamplerCreateInfo sampler_create_info{};

			pipeline.sampler = SDL_CreateGPUSampler(device.gpu, &sampler_create_info);
			pipeline.pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

			SDL_ReleaseGPUShader(device.gpu, vert_shader);
			SDL_ReleaseGPUShader(device.gpu, frag_shader);
		});

	world.system<RenderDevice, UiTextPipeline>("create ui text pipeline")
		.kind(Phases::OnStart)
		.each([&world](RenderDevice& device, UiTextPipeline& pipeline) {
			constexpr bool use_sdf = true;

			auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/text.vert.msl", 1);
			auto frag_shader = load_shader(*device.gpu, use_sdf ? "assets/shaders/out/text_sdf.frag.msl" : "assets/shaders/out/text.frag.msl", 0, 1);

			SDL_GPUColorTargetDescription color_target_description = {
				.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, world.get<WindowModule>().main_window),
				.blend_state = {
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.color_write_mask = 0xF,
					.enable_blend = true,
				}
			};

			std::array<SDL_GPUVertexAttribute, 3> vertex_attributes = {
				SDL_GPUVertexAttribute{
					.location = 0,
					.buffer_slot = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = offsetof(Vertex, position),
				},
				SDL_GPUVertexAttribute{
					.location = 1,
					.buffer_slot = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = offsetof(Vertex, color),
				},
				SDL_GPUVertexAttribute{
					.location = 2,
					.buffer_slot = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
					.offset = offsetof(Vertex, uv),
				}
			};

			SDL_GPUVertexBufferDescription vertex_buffer_description = {
				.slot = 0,
				.pitch = sizeof(Vertex),
				.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				.instance_step_rate = 0,
			};

			SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
				.vertex_shader = vert_shader,
				.fragment_shader = frag_shader,
				.vertex_input_state = {
					.vertex_buffer_descriptions = &vertex_buffer_description,
					.num_vertex_buffers = 1,
					.vertex_attributes = vertex_attributes.data(),
					.num_vertex_attributes = vertex_attributes.size(),
				},
				.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				.target_info = {
					.color_target_descriptions = &color_target_description,
					.num_color_targets = 1,
					.depth_stencil_format =
						SDL_GPU_TEXTUREFORMAT_INVALID, /* Need to set this to avoid
															missing initializer for
															field error */
					.has_depth_stencil_target = false,
				},
			};

			SDL_GPUSamplerCreateInfo sampler_info = {
				.min_filter = SDL_GPU_FILTER_LINEAR,
				.mag_filter = SDL_GPU_FILTER_LINEAR,
				.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
				.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
				.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
				.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
			};

			pipeline.sampler = SDL_CreateGPUSampler(device.gpu, &sampler_info);
			pipeline.pipeline = SDL_CreateGPUGraphicsPipeline(device.gpu, &pipeline_create_info);

			SDL_ReleaseGPUShader(device.gpu, vert_shader);
			SDL_ReleaseGPUShader(device.gpu, frag_shader);

			pipeline.engine = TTF_CreateGPUTextEngine(device.gpu);
		});

	world.system<RenderDevice, CopyCommands, UiStorage>("init ui buffers")
		.kind(Phases::OnStart)
		.each([](RenderDevice& device, CopyCommands& commands, UiStorage& storage) {
			std::vector<uint16_t> indices;
			indices.reserve(max_ui_per_batch * 6);

			for (uint32_t i = 0; i < max_ui_per_batch; ++i) {
				uint16_t base = i * 4;

				indices.push_back(base + 0);
				indices.push_back(base + 1);
				indices.push_back(base + 2);
				indices.push_back(base + 2);
				indices.push_back(base + 3);
				indices.push_back(base + 0);
			}

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = max_ui_per_batch * 4 * sizeof(Vertex),
			};

			storage.transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

			const auto verticies_byte_szie = max_ui_per_batch * 4 * sizeof(Vertex);
			const auto indecies_byte_szie = indices.size() * sizeof(indices[0]);

			SDL_GPUBufferCreateInfo vertex_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = verticies_byte_szie,
			};
			SDL_GPUBufferCreateInfo index_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_INDEX,
				.size = static_cast<Uint32>(indecies_byte_szie),
			};

			storage.vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
			storage.index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);

			// TODO: use storage transfer buf?
			SDL_GPUTransferBufferCreateInfo indecies_transfer_buffer_create_info{
				.usage = SDL_GPUTransferBufferUsage::SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<uint32_t>(indecies_byte_szie)
			};

			auto transfer_buf = SDL_CreateGPUTransferBuffer(device.gpu, &indecies_transfer_buffer_create_info);

			auto transfer_mem = SDL_MapGPUTransferBuffer(device.gpu ,transfer_buf, false);

			std::memcpy(transfer_mem, indices.data(), indecies_byte_szie);

			SDL_UnmapGPUTransferBuffer(device.gpu, transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(device.gpu);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

			SDL_GPUTransferBufferLocation index_buffer_location{ .transfer_buffer = transfer_buf };

			SDL_GPUBufferRegion index_buffer_region{ .buffer = storage.index_buffer, .size = static_cast<uint32_t>(indecies_byte_szie) };

			SDL_UploadToGPUBuffer(copy_pass, &index_buffer_location, &index_buffer_region, false);

			SDL_EndGPUCopyPass(copy_pass);

			assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());

			SDL_ReleaseGPUTransferBuffer(device.gpu, transfer_buf);
		});

	world.system<RenderDevice, UiTextStorage>("init ui text buffers")
		.kind(Phases::OnStart)
		.each([](RenderDevice& device, UiTextStorage& storage) {
			SDL_GPUBufferCreateInfo vertex_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = sizeof(Vertex) * UI_TEXT_MAX_VERTEX_COUNT,
			};

			SDL_GPUBufferCreateInfo index_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_INDEX,
				.size = sizeof(int) * UI_TEXT_MAX_INDEX_COUNT,
			};

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = (sizeof(Vertex) * UI_TEXT_MAX_VERTEX_COUNT) + (sizeof(int) * UI_TEXT_MAX_INDEX_COUNT)
			};

			storage.vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
			storage.index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);
			storage.transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);
		});

	world.system<Image, GlobalTransform, Aabb>()
		.kind(Phases::Update)
		.each([](Image& image, GlobalTransform& transform, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + image.texture->get_size();
		});

	world.system<Node, BackgroundColor, GlobalTransform, Aabb>()
		.without<Image>()
		.kind(Phases::Update)
		.each([](Node& node, BackgroundColor& color, GlobalTransform& transform, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + node.size;
		});

	world.system<TextData, GlobalTransform, Aabb>()
		.with<Text>()
		.kind(Phases::Update)
		.each([](TextData& data, GlobalTransform& transform, Aabb& aabb) {
			aabb.min = transform.translation;
			aabb.max = glm::vec2(transform.translation) + glm::vec2(data.size);
		});

	auto image_query = world.query<Node, Image, GlobalTransform, Visible2d>();

	auto color_query = world.query_builder<Node, GlobalTransform, Visible2d, BackgroundColor>()
		.without<Image>()
		.build();

	auto text_query = world.query<Node, Text, TextFont, TextData, TextColor, GlobalTransform, Visible2d>();

	world.system<VisibleEntities, CameraRenderPhaseItems, CameraCollectedUiItems>("collect node images")
		.with<Camera>()
		.kind(Phases::CollectRenderData)
		.each([transparent_ui, image_query](flecs::entity camera, VisibleEntities& visible_entities, CameraRenderPhaseItems& render_items, CameraCollectedUiItems& ui_items) {
			image_query.each([&](flecs::entity entity, Node& node, Image& image, GlobalTransform& transform, Visible2d& visible) {
				if (!visible.value) {
					return;
				}

				if (!visible_entities.entities.contains(entity)) {
					return;
				}

				render_items[camera][transparent_ui].emplace_back(entity, &draw, static_cast<float>(node.stack_index));

				ui_items[camera].lookup[entity] = ui_items[camera].items.size();
				ui_items[camera].items.emplace_back(entity, image.texture, transform, image.color, image.texture->get_size());
			});
		});

	world.system<VisibleEntities, CameraRenderPhaseItems, CameraCollectedUiItems, WhiteTexture>("collect node colors")
		.with<Camera>()
		.kind(Phases::CollectRenderData)
		.each([transparent_ui, color_query](flecs::entity camera, VisibleEntities& visible_entities, CameraRenderPhaseItems& render_items, CameraCollectedUiItems& ui_items, WhiteTexture& white_texture) {
			color_query.each([&](flecs::entity entity, Node& node, GlobalTransform& transform, Visible2d& visible, BackgroundColor& color) {
				if (node.size == glm::vec2{ 0.f, 0.f } || !visible.value) {
					return;
				}

				if (!visible_entities.entities.contains(entity)) {
					return;
				}

				render_items[camera][transparent_ui].emplace_back(entity, &draw, static_cast<float>(node.stack_index));

				ui_items[camera].lookup[entity] = ui_items[camera].items.size();
				ui_items[camera].items.emplace_back(entity, white_texture.texture, transform, color, node.size);
			});
		});

	world.system<VisibleEntities, CameraRenderPhaseItems, CameraCollectedUiTextItems>("collect text nodes")
		.with<Camera>()
		.kind(Phases::CollectRenderData)
		.each([transparent_ui, text_query](flecs::entity camera, VisibleEntities& visible_entities, CameraRenderPhaseItems& render_items, CameraCollectedUiTextItems& text_items) {
			text_query.each([&](flecs::entity entity, Node& node, Text& text, TextFont& font, TextData& data, TextColor& color, GlobalTransform& transform, Visible2d& visible) {
				if (!visible.value) {
					return;
				}

				if (!visible_entities.entities.contains(entity)) {
					return;
				}

				render_items[camera][transparent_ui].emplace_back(entity, &draw_ui_text, static_cast<float>(node.stack_index));

				text_items[camera].lookup[entity] = text_items[camera].items.size();
				text_items[camera].items.emplace_back(entity, nullptr, transform, color, data.ttf_data, font.size / font.handle->get_size());
			});
		});

	world.system<CameraRenderPhaseItems, CameraCollectedUiItems, CameraUiBatches, UiStorage, RenderDevice, CopyCommands>("generate ui batches")
		.kind(Phases::PrepareRenderData)
		.each([transparent_ui](CameraRenderPhaseItems& camera_render_items, CameraCollectedUiItems& camera_ui_items, CameraUiBatches& camera_batches, UiStorage& storage, RenderDevice& device, CopyCommands& copy_commands) {
			camera_batches.clear();

			auto vertices = static_cast<Vertex*>(SDL_MapGPUTransferBuffer(device.gpu, storage.transfer_buffer, false));

			uint32_t current_vertex = 0;

			for (auto& [camera, render_items] : camera_render_items) {
				auto& phase_items = render_items[transparent_ui];
				auto& ui_items = camera_ui_items[camera];
				auto& batches = camera_batches[camera];

				if (phase_items.empty() || ui_items.items.empty()) {
					continue;
				}

				flecs::entity_t current_batch_entity = flecs::entity::null();
				size_t current_batch_index = 0;

				for (size_t i = 0; i < phase_items.size(); ++i) {
					const auto& data = phase_items[i];

					if (!ui_items.lookup.contains(data.entity)) {
						current_batch_entity = flecs::entity::null();

						continue;
					}

					const auto& ui_node = ui_items.items[ui_items.lookup[data.entity]];

					if (!current_batch_entity || batches.at(current_batch_entity).texture != ui_node.texture || batches.at(current_batch_entity).size >= max_ui_per_batch) {
						current_batch_entity = ui_node.entity;
						current_batch_index = i;

						batches.emplace(current_batch_entity, UiBatch{
							.size = 0,
							.first_index = current_vertex / 4 * 6,
							.texture = ui_node.texture,
						});
					}

					auto& current_batch = batches.at(current_batch_entity);

					const auto pos = ui_node.transform.translation;
					const auto color = ui_node.color;
					const auto size = ui_node.size;

					vertices[current_vertex + 0] = { { pos.x         , pos.y + size.y, 0.f }, color, { 0.f, 1.f } };
					vertices[current_vertex + 1] = { { pos.x + size.x, pos.y + size.y, 0.f }, color, { 1.f, 1.f } };
					vertices[current_vertex + 2] = { { pos.x + size.x, pos.y         , 0.f }, color, { 1.f, 0.f } };
					vertices[current_vertex + 3] = { { pos.x         , pos.y         , 0.f }, color, { 0.f, 0.f } };

					current_vertex += 4;

					++current_batch.size;
					++phase_items[current_batch_index].batch_size;

				}

				ui_items.items.clear();
				ui_items.lookup.clear();
			}

			if (current_vertex == 0) {
				return;
			}

			SDL_UnmapGPUTransferBuffer(device.gpu, storage.transfer_buffer);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_commands.buffer);

			const auto buffer_location = SDL_GPUTransferBufferLocation{
				.transfer_buffer = storage.transfer_buffer,
			};

			const auto buffer_region = SDL_GPUBufferRegion{
				.buffer = storage.vertex_buffer,
				.size = static_cast<Uint32>(current_vertex * sizeof(Vertex))
			};

			SDL_UploadToGPUBuffer(copy_pass,  &buffer_location, &buffer_region, false);

			SDL_EndGPUCopyPass(copy_pass);
		});

	world.system<CameraCollectedUiTextItems, CameraRenderPhaseItems, UiTextStorage, CameraUiTextBatches, RenderDevice, CopyCommands>()
		.kind(Phases::PrepareRenderData)
		.each([transparent_ui](CameraCollectedUiTextItems& camera_text_items, CameraRenderPhaseItems& camera_render_items, UiTextStorage& storage, CameraUiTextBatches& camera_batches, RenderDevice& device, CopyCommands& commands) {
			camera_batches.clear();

			auto transfer_buffer = SDL_MapGPUTransferBuffer(device.gpu, storage.transfer_buffer, false);

			auto vertices = static_cast<Vertex*>(transfer_buffer);
			auto indices = reinterpret_cast<int*>(vertices + UI_TEXT_MAX_VERTEX_COUNT);

			auto vertex_count = 0;
			auto index_count = 0;

			for (auto& [camera, render_items] : camera_render_items) {
				auto& phase_items = render_items[transparent_ui];
				auto& text_items = camera_text_items[camera];
				auto& batches = camera_batches[camera];

				if (phase_items.empty() || text_items.items.empty()) {
					continue;
				}

				flecs::entity_t current_batch_entity = flecs::entity::null();
				size_t current_batch_index = 0;

				for (size_t i = 0; i < phase_items.size(); ++i) {
					const auto& data = phase_items[i];

					if (!text_items.lookup.contains(data.entity)) {
						current_batch_entity = flecs::entity::null();

						continue;
					}

					auto& text_data = text_items.items[text_items.lookup[data.entity]];

					auto seq = TTF_GetGPUTextDrawData(text_data.ttf_data);

					if (!seq) {
						current_batch_entity = flecs::entity::null();

						continue;
					}

					if (!current_batch_entity || batches.at(current_batch_entity).back().texture != seq->atlas_texture) {
						current_batch_entity = text_data.entity;
						current_batch_index = i;

						batches[current_batch_entity].emplace_back(
							static_cast<size_t>(index_count),
							0,
							seq->atlas_texture
						);
					}

					auto& current_batch = batches.at(current_batch_entity).back();

					while (seq) {
						if (seq->atlas_texture != current_batch.texture) {
							current_batch_entity = text_data.entity;
							current_batch_index = i;

							batches[current_batch_entity].emplace_back(
								static_cast<size_t>(index_count),
								0,
								seq->atlas_texture
							);

							current_batch = batches.at(current_batch_entity).back();
						}

						for (int j = 0; j < seq->num_vertices; j++) {
							const auto pos = seq->xy[j];
							const auto uv = seq->uv[j];

							vertices[vertex_count + j] = Vertex{
								.position{ text_data.transform.translation + glm::vec3{ pos.x, -pos.y, 0.f } * text_data.scale },
								.color = text_data.color,
								.uv{ uv.x, uv.y },
							};
						}

						for (int j = 0; j < seq->num_indices; j++) {
							indices[index_count + j] = seq->indices[j] + vertex_count;
						}

						vertex_count += seq->num_vertices;
						index_count += seq->num_indices;

						current_batch.num_indices += seq->num_indices;

						seq = seq->next;
					}

					++phase_items[current_batch_index].batch_size;
				}

				text_items.items.clear();
				text_items.lookup.clear();
			}

			if (vertex_count == 0 || index_count == 0) {
				return;
			}

			SDL_UnmapGPUTransferBuffer(device.gpu, storage.transfer_buffer);

			auto copy_pass = SDL_BeginGPUCopyPass(commands.buffer);

			SDL_GPUTransferBufferLocation vertex_transfer_buffer_location{
				.transfer_buffer = storage.transfer_buffer,
				.offset = 0 
			};
			SDL_GPUBufferRegion vertex_buffer_location{
				.buffer = storage.vertex_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(Vertex) * vertex_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_location, false);

			SDL_GPUTransferBufferLocation index_transfer_buffer_location {
				.transfer_buffer = storage.transfer_buffer,
				.offset = static_cast<Uint32>(sizeof(Vertex) * UI_TEXT_MAX_VERTEX_COUNT)
			};
			SDL_GPUBufferRegion index_buffer_region {
				.buffer = storage.index_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(int) * index_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);

			SDL_EndGPUCopyPass(copy_pass);
		});

	world.add<UiPipeline>();
	world.add<UiStorage>();
	world.add<CameraUiBatches>();
	world.add<CameraCollectedUiItems>();
	world.add<UiTextPipeline>();
	world.add<UiTextStorage>();
	world.add<CameraUiTextBatches>();
	world.add<CameraCollectedUiTextItems>();
}
