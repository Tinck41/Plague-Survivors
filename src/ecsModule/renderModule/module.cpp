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

	world.system<Window, RenderDevice>()
		.kind(Phases::OnStart)
		.each([](Window& window, RenderDevice& render_device) {
			auto gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);

			assert(gpu && SDL_GetError());
			assert(SDL_ClaimWindowForGPUDevice(gpu, window.handle) && SDL_GetError());

			render_device.gpu = gpu;
		});

	world.system<RenderDevice, WhiteTexture>()
		.kind(Phases::OnStart)
		.each([](RenderDevice& render_device, WhiteTexture& white_texture) {
			auto texture_create_info = SDL_GPUTextureCreateInfo{
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
				.width = 1,
				.height = 1,
				.layer_count_or_depth = 1,
				.num_levels = 1,
			};

			auto texture = SDL_CreateGPUTexture(render_device.gpu, &texture_create_info);

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = 4,
			};
			auto tex_transfer_buf = SDL_CreateGPUTransferBuffer(render_device.gpu, &transfer_buffer_create_info);
			auto tex_transfer_mem = SDL_MapGPUTransferBuffer(render_device.gpu, tex_transfer_buf, false);

			uint32_t white_pixel = 0xffffffff;
			std::memcpy(tex_transfer_mem, &white_pixel, 4);

			SDL_UnmapGPUTransferBuffer(render_device.gpu, tex_transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(render_device.gpu);
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

			SDL_ReleaseGPUTransferBuffer(render_device.gpu, tex_transfer_buf);

			white_texture.texture = std::make_shared<Texture>(render_device.gpu, texture, glm::vec2{ 1.f, 1.f });
		});

	world.system<RenderDevice, CopyCommands>()
		.kind(Phases::PostUpdate)
		.each([](RenderDevice& device, CopyCommands& copy_commands) {
			copy_commands.buffer = SDL_AcquireGPUCommandBuffer(device.gpu);
		});

	world.system<RenderDevice, CopyCommands>()
		.kind(Phases::PreRender)
		.each([](RenderDevice& device, CopyCommands& copy_commands) {
			SDL_SubmitGPUCommandBuffer(copy_commands.buffer);
		});

	world.system<RenderDevice, RenderCommands>()
		.kind(Phases::Clear)
		.each([](RenderDevice& device, RenderCommands& render_commands) {
			render_commands.cmd_buffer = SDL_AcquireGPUCommandBuffer(device.gpu);
		});

	world.system<RenderPass, Window, RenderDevice, RenderCommands>()
		.kind(Phases::Clear)
		.each([](RenderPass& pass, Window& window, RenderDevice& device, RenderCommands& render_commands) {
			if (!pass.target) {
				assert(SDL_WaitAndAcquireGPUSwapchainTexture(render_commands.cmd_buffer, window.handle, &pass.target, nullptr, nullptr) && SDL_GetError());
			}

			auto color_target = SDL_GPUColorTargetInfo{
				.texture = pass.target,
				.clear_color = { 0.f, 0.f, 0.f, 1.f },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE
			};

			pass.render_pass = SDL_BeginGPURenderPass(render_commands.cmd_buffer, &color_target, 1, nullptr);

			SDL_PushGPUDebugGroup(render_commands.cmd_buffer, "render");
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

	world.system<VisibleEntities, CameraRenderPhaseItems, RenderStats>("new render")
		.with<Camera>()
		.kind(Phases::Render)
		.each([&world](flecs::entity_t camera, VisibleEntities& visible_entities, CameraRenderPhaseItems& phase_items, RenderStats& stats) {
			stats.draw_calls = 0;

			for (const auto& phase : render_phases_order) {
				if (!phase_items.contains(camera) || !phase_items.at(camera).contains(phase) || phase_items.at(camera).at(phase).empty()) {
					continue;
				}

				auto& items = phase_items.at(camera).at(phase);

				size_t i = 0;

				while (i < items.size()) {
					const auto& item = items[i];

					item.draw_function(camera, item.entity, world);
					i += item.batch_size > 0 ? item.batch_size : 1;

					++stats.draw_calls;
				}

				items.clear();
			}
		});

	world.system<RenderPass, RenderCommands>()
		.kind(Phases::Display)
		.each([](RenderPass& pass, RenderCommands& render_commands) {
			SDL_EndGPURenderPass(pass.render_pass);
			SDL_PopGPUDebugGroup(render_commands.cmd_buffer);
		});

	world.system<RenderCommands>()
		.kind(Phases::Display)
		.each([](RenderCommands& render_commands) {
			assert(SDL_SubmitGPUCommandBuffer(render_commands.cmd_buffer) && SDL_GetError());
		});

	world.add<RenderDevice>();
	world.add<CopyCommands>();
	world.add<RenderCommands>();
	world.add<WhiteTexture>();
	world.add<CameraRenderPhaseItems>();
	world.add<RenderStats>();
}
