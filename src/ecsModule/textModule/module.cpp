#include "module.h"

#include "ecsModule/appModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/meshModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ext/matrix_transform.hpp"
#include "utils/sdl.h"
#include "font.h"

using namespace ps;

static std::map<flecs::entity_t, std::shared_ptr<Texture>> font_textures;

TextModule::TextModule(flecs::world& world) {
	world.module<TextModule>();

	world.import<RenderModule>();
	world.import<SpriteModule>();
	world.import<TransformModule>();

	world.component<TextStorage>().add(flecs::Singleton);
	world.component<TextPipeline>().add(flecs::Singleton);
	world.component<Text>()
		.add(flecs::With, world.component<Transform>());

	world.system<Application, RenderDevice, TextPipeline>()
		.kind(Phases::OnStart)
		.each([](Application& app, RenderDevice& device, TextPipeline& pipeline) {
			constexpr bool use_sdf = true;

			auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/text.vert.msl", 1);
			auto frag_shader = load_shader(*device.gpu, use_sdf ? "assets/shaders/out/text_sdf.frag.msl" : "assets/shaders/out/text.frag.msl", 0, 1);

			SDL_GPUColorTargetDescription color_target_description = {
				.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, app.window),
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

	world.system<RenderDevice, TextStorage>()
		.kind(Phases::OnStart)
		.each([](RenderDevice& device, TextStorage& storage) {
			constexpr auto max_indecies = 100'000;
			constexpr auto max_vertices = 100'000;

			SDL_GPUBufferCreateInfo vertex_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = sizeof(Vertex) * max_vertices,
			};

			SDL_GPUBufferCreateInfo index_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_INDEX,
				.size = sizeof(int) * max_indecies,
			};

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = (sizeof(Vertex) * max_vertices) + (sizeof(int) * max_indecies)
			};

			storage.vertex_buffer = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
			storage.index_buffer = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);
			storage.transfer_buffer = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);
		});

	world.observer<Text, TextStorage, TextPipeline>()
		.event(flecs::OnSet)
		.each([](flecs::entity e, Text& text, TextStorage& storage, TextPipeline& pipeline){
			storage.text_data[text.string] = TTF_CreateText(pipeline.engine, *text.font, text.string.c_str(), text.string.size());
		});

	world.system<const Text, TextStorage, TextPipeline, RenderDevice, CopyCommands>()
		.kind(Phases::PostUpdate)
		.each([world](flecs::entity e, const Text& text, TextStorage& storage, TextPipeline& pipeline, RenderDevice& device, CopyCommands& commands) {
			auto data = TTF_GetGPUTextDrawData(storage.text_data.at(text.string));

			auto indices = static_cast<int*>(SDL_calloc(10'000, sizeof(int)));

			auto transfer_data = static_cast<Vertex*>(SDL_MapGPUTransferBuffer(device.gpu, storage.transfer_buffer, false));
			auto vertex_count = 0;
			auto index_count = 0;

			for (auto seq = data; seq != nullptr; seq = seq->next) {
				for (int i = 0; i < seq->num_vertices; i++) {
					const auto pos = seq->xy[i];
					const auto uv = seq->uv[i];

					Vertex vert{
						.position{ pos.x, pos.y, 0.f },
						.uv{ uv.x, uv.y }
					};

					transfer_data[vertex_count + i] = vert;
				}

				SDL_memcpy(indices + index_count, seq->indices, seq->num_indices * sizeof(int));

				vertex_count += seq->num_vertices;
				index_count += seq->num_indices;
			}

			SDL_memcpy(transfer_data + 10'000, indices, sizeof(int) * index_count);

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
				.offset = sizeof(Vertex) * 10'000
			};
			SDL_GPUBufferRegion index_buffer_region {
				.buffer = storage.index_buffer,
				.offset = 0,
				.size = static_cast<Uint32>(sizeof(int) * index_count)
			};

			SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);

			SDL_EndGPUCopyPass(copy_pass);
		});

	world.system<const Text, const GlobalTransform, const TextStorage, TextPipeline, RenderDevice, RenderCommands>()
		.kind(Phases::Render)
		.each([world](flecs::entity e, const Text& text, const GlobalTransform& transform, const TextStorage& storage, TextPipeline& pipeline, const RenderDevice& device, const RenderCommands& commands) {
			auto data = TTF_GetGPUTextDrawData(storage.text_data.at(text.string));

			SDL_GPUBufferBinding vertex_bindings{
				.buffer = storage.vertex_buffer, .offset = 0
			};
			SDL_GPUBufferBinding index_bindings{
				.buffer = storage.index_buffer, .offset = 0
			};

			const auto camera = world.entity(CameraModule::EcsCamera);
			const auto view = glm::translate(glm::mat4(1.f), -camera.get<GlobalTransform>().translation);
			const auto view_proj = view * camera.get<Camera>().projection;

			int tw, th;
			TTF_GetTextSize(storage.text_data.at(text.string), &tw, &th);

			std::array<glm::mat4, 2> matrices{
				view_proj,
				glm::scale(transform.matrix, glm::vec3(1.f, -1.f, 1.f))
			};


			SDL_BindGPUGraphicsPipeline(commands.render_pass, pipeline.pipeline);
			SDL_BindGPUVertexBuffers(commands.render_pass, 0, &vertex_bindings, 1);
			SDL_BindGPUIndexBuffer(commands.render_pass, &index_bindings, SDL_GPU_INDEXELEMENTSIZE_32BIT);
			SDL_PushGPUVertexUniformData(commands.cmd_buffer, 0, &matrices, sizeof(glm::mat4) * 2);

			int index_offset = 0;
			int vertex_offset = 0;

			for (TTF_GPUAtlasDrawSequence *seq = data; seq != nullptr; seq = seq->next) {
				SDL_GPUTextureSamplerBinding bindings{
					.texture = seq->atlas_texture,
					.sampler = pipeline.sampler,
				};

				SDL_BindGPUFragmentSamplers(commands.render_pass, 0, &bindings, 1);

				SDL_DrawGPUIndexedPrimitives(commands.render_pass, seq->num_indices, 1, index_offset, vertex_offset, 0);

				index_offset += seq->num_indices;
				vertex_offset += seq->num_vertices;
			}
		});

	TTF_Init();

	world.add<TextStorage>();
	world.add<TextPipeline>();
}
