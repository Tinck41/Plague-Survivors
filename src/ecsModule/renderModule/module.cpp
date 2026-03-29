#include "module.h"

#include "SDL3_image/SDL_image.h"
#include "SDL3/SDL.h"
#include "ecsModule/cameraModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/windowModule/components.h"
#include "ecsModule/windowModule/module.h"
#include "ext/matrix_clip_space.hpp"
#include "ext/matrix_transform.hpp"
#include "spdlog/spdlog.h"
#include "utils/sdl.h"
#include "utils/visit.h"
#include <algorithm>
#include <ranges>

using namespace ps;

RenderModule::RenderModule(flecs::world& world) {
	world.module<RenderModule>();

	world.import<WindowModule>();
	world.import<TransformModule>();
	world.import<CameraModule>();

	world.component<CopyCommands>().add(flecs::Singleton);
	world.component<RenderCommands>().add(flecs::Singleton);
	world.component<RenderDevice>().add(flecs::Singleton);
	world.component<WhiteTexture>().add(flecs::Singleton);
	world.component<CameraRenderPhaseItems>().add(flecs::Singleton);
	world.component<CameraCompositionPipeline>().add(flecs::Singleton);
	world.component<RenderStats>().add(flecs::Singleton);

	world.component<RenderPass>();

	world.component<Camera>()
		.add(flecs::With, world.component<RenderPass>());

	world.component<SDL_Color>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a");

	world.component<RenderPhase>();
	world.component<BindData>();
	world.component<BindTexture>();

	world.system("check render phase duplicates")
		.kind(Phases::OnStart)
		.each([] {
			// TODO
		});

	world.observer<RenderDevice>()
		.event(flecs::OnSet)
		.each([&world](RenderDevice& render_device) {
			world.each([&render_device](Window& window) {
				assert(SDL_ClaimWindowForGPUDevice(render_device.gpu, window.handle) && SDL_GetError());
			});
		});

	// TODO: sometimes overlap with observer from WindowModule
	world.observer<Window, RenderDevice>()
		.event(flecs::OnSet)
		.each([](Window& window, RenderDevice& render_device) {
			assert(SDL_ClaimWindowForGPUDevice(render_device.gpu, window.handle) && SDL_GetError());
		});

	world.observer<CameraCompositionPipeline, RenderDevice>()
		.term_at(1).filter()
		.event(flecs::OnAdd)
		.each([&world](CameraCompositionPipeline& pipeline, RenderDevice& device) {
			auto vert_shader = load_shader(*device.gpu, "assets/shaders/out/camera_composition.vert.msl", 1);
			auto frag_shader = load_shader(*device.gpu, "assets/shaders/out/camera_composition.frag.msl", 0, 1);

			SDL_GPUColorTargetDescription color_target_description{
				.format = SDL_GetGPUSwapchainTextureFormat(device.gpu, world.get<WindowModule>().main_window),
				.blend_state = {
					.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.color_blend_op = SDL_GPU_BLENDOP_ADD,
					.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
					.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					.color_write_mask = 0xF,
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
					//.vertex_buffer_descriptions = &vertex_buffer_description,
					//.num_vertex_buffers = 1,
					//.vertex_attributes = vertex_attrs.data(),
					//.num_vertex_attributes = vertex_attrs.size(),
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

	world.system<RenderDevice, WhiteTexture>()
		.kind(Phases::OnStart)
		.each([](RenderDevice& render_device, WhiteTexture& white_texture) {
			white_texture.texture = std::make_shared<Texture>(render_device.gpu, glm::vec2{ 1.f, 1.f }, WHITE);
		});

	world.system<RenderDevice, CopyCommands>()
		.kind(Phases::PostUpdate)
		.each([](RenderDevice& device, CopyCommands& copy_commands) {
			copy_commands.buffer = SDL_AcquireGPUCommandBuffer(device.gpu);
		});

	world.system<RenderDevice, CopyCommands, RenderCommands>()
		.kind(Phases::Clear)
		.each([](RenderDevice& device, CopyCommands& copy_commands, RenderCommands& render_commands) {
			SDL_SubmitGPUCommandBuffer(copy_commands.buffer);
			render_commands.cmd_buffer = SDL_AcquireGPUCommandBuffer(device.gpu);

		});

	world.system<Window, RenderCommands>()
		.kind(Phases::Clear)
		.each([&world](Window& window, RenderCommands& render_commands) {
			assert(SDL_WaitAndAcquireGPUSwapchainTexture(render_commands.cmd_buffer, window.handle, &window.swapchain_texture, nullptr, nullptr) && SDL_GetError());
		});

	world.system<Window, RenderDevice, RenderCommands>()
		.with<MainWindow>()
		.kind(Phases::Clear)
		.each([](Window& main_window, RenderDevice& device, RenderCommands& render_commands) {
			auto color_target = SDL_GPUColorTargetInfo{
				.texture = main_window.swapchain_texture,
				.clear_color = BLACK,
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE
			};

			auto clear_pass =  SDL_BeginGPURenderPass(render_commands.cmd_buffer, &color_target, 1, nullptr);
			SDL_EndGPURenderPass(clear_pass);
		});

	world.system<CameraRenderPhaseItems>("sort render data")
		.kind(Phases::SortRenderData)
		.each([](CameraRenderPhaseItems& render_items) {
			for (auto& camera_items : render_items| std::ranges::views::values) {
				for (auto& items : camera_items| std::ranges::views::values) {
					std::ranges::sort(items, [](const RenderPhase& lhs, const RenderPhase& rhs) {
						if (lhs.sort_value == rhs.sort_value) {
							return lhs.entity < rhs.entity;
						}
						return lhs.sort_value < rhs.sort_value;
					});
				}
			}
		});

	world.system<RenderStats>()
		.kind(Phases::PreRender)
		.each([](RenderStats& stats) {
			stats.draw_calls = 0;
		});

	world.system<Camera, RenderPass, RenderCommands, CameraRenderPhaseItems, RenderStats>()
		.kind(Phases::Render)
		.each([&world](flecs::entity camera_entity, Camera& camera, RenderPass& pass, RenderCommands& render_commands, CameraRenderPhaseItems& phase_items, RenderStats& stats) {
			auto render_texture = visit(camera.render_target, visitors{
				[&world](flecs::entity_t window_entity) {
					return world.entity(window_entity).get<Window>().swapchain_texture;
				},
				[](std::shared_ptr<Texture> texture) {
					return &texture->get_gpu_texture();
				},
				[](auto&&) -> SDL_GPUTexture* {
					throw std::runtime_error("invalid render target");
				}
			});

			auto color_target = SDL_GPUColorTargetInfo{
				.texture = render_texture,
				.clear_color = camera.clear_color,
				.load_op = camera.load_op,
				.store_op = SDL_GPU_STOREOP_STORE
			};

			pass.render_pass = SDL_BeginGPURenderPass(render_commands.cmd_buffer, &color_target, 1, nullptr);

			SDL_PushGPUDebugGroup(render_commands.cmd_buffer, std::format("camera: {}", camera_entity.name() ? std::string(camera_entity.name()) : std::to_string(camera_entity.id())).c_str());

			for (const auto& phase : render_phases_order) {
				if (!phase_items.contains(camera_entity) || !phase_items.at(camera_entity).contains(phase) || phase_items.at(camera_entity).at(phase).empty()) {
					continue;
				}

				auto& items = phase_items.at(camera_entity).at(phase);

				size_t i = 0;

				while (i < items.size()) {
					const auto& item = items[i];

					item.draw_function(camera_entity, item.entity, world);
					i += item.batch_size > 0 ? item.batch_size : 1;

					++stats.draw_calls;
				}

				items.clear();
			}

			SDL_EndGPURenderPass(pass.render_pass);
			SDL_PopGPUDebugGroup(render_commands.cmd_buffer);
		});

	world.system<CameraCompositionGraph, RenderCommands, CameraCompositionPipeline>()
		.kind(Phases::Render)
		.each([&world](CameraCompositionGraph& graph, RenderCommands& render_commands, CameraCompositionPipeline& pipeline) {
			SDL_GPUTexture* swapchain_texture = nullptr;

			world.each([&swapchain_texture](Window& window) {
				swapchain_texture = window.swapchain_texture;
			});

			auto color_target = SDL_GPUColorTargetInfo{
				.texture = swapchain_texture,
				.clear_color = TRANSPARENT,
				.load_op = SDL_GPU_LOADOP_LOAD,
				.store_op = SDL_GPU_STOREOP_STORE
			};

			auto render_pass = SDL_BeginGPURenderPass(render_commands.cmd_buffer, &color_target, 1, nullptr);

			auto cameras = graph.topological_sort();

			for (const auto& camera : cameras) {
				if (!camera.enabled()) {
					continue;
				}
				const auto& camera_data = world.entity(camera).get<Camera>();
				const auto& transform = world.entity(camera).get<GlobalTransform>();

				if (!std::holds_alternative<std::shared_ptr<Texture>>(camera_data.render_target)) {
					continue;
				}

				SDL_PushGPUDebugGroup(render_commands.cmd_buffer, std::format("composing camera: {}", camera.name() ? std::string(camera.name()) : std::to_string(camera.id())).c_str());

				const auto texture = std::get<std::shared_ptr<Texture>>(camera_data.render_target);

				SDL_GPUViewport viewport{
					.x = transform.translation.x,
					.y = transform.translation.y,
					.w = texture->get_size().x,
					.h = texture->get_size().y,
					.min_depth = 0.f,
					.max_depth = 1.f,
				};

				SDL_SetGPUViewport(render_pass, &viewport);

				SDL_GPUTextureSamplerBinding binding{
					.texture = &texture->get_gpu_texture(),
					.sampler = pipeline.sampler,
				};

				SDL_BindGPUGraphicsPipeline(render_pass, pipeline.pipeline);
				SDL_BindGPUFragmentSamplers(render_pass, 0, &binding, 1);

				SDL_DrawGPUPrimitives(render_pass, 6, 1, 0, 0);

				SDL_PopGPUDebugGroup(render_commands.cmd_buffer);
			}

			SDL_EndGPURenderPass(render_pass);
		});

	world.system<RenderCommands>()
		.kind(Phases::Display)
		.each([](RenderCommands& render_commands) {
			assert(SDL_SubmitGPUCommandBuffer(render_commands.cmd_buffer) && SDL_GetError());
		});

	auto gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);

	assert(gpu && SDL_GetError());

	world.set<RenderDevice>({ gpu });

	world.add<CopyCommands>();
	world.add<RenderCommands>();
	world.add<WhiteTexture>();
	world.add<CameraRenderPhaseItems>();
	world.add<RenderStats>();
}
