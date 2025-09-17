#include "module.h"

#include "SDL3_image/SDL_image.h"
#include "SDL3/SDL.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/appModule/module.h"
#include "ecsModule/cameraModule/module.h"
#include "ext/matrix_clip_space.hpp"
#include "ext/matrix_transform.hpp"
#include "spdlog/spdlog.h"
#include "utils/sdl.h"

using namespace ps;

RenderModule::RenderModule(flecs::world& world) {
	world.module<RenderModule>();

	world.import<AppModule>();
	world.import<TransformModule>();
	world.import<CameraModule>();

	world.component<CopyCommands>().add(flecs::Singleton);
	world.component<RenderCommands>().add(flecs::Singleton);
	world.component<RenderDevice>().add(flecs::Singleton);
	world.component<WhiteTexture>().add(flecs::Singleton);

	world.component<SDL_Color>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a");

	world.system<Application, RenderDevice>()
		.kind(Phases::OnStart)
		.each([](Application& app, RenderDevice& render_device) {
			auto gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, nullptr);

			assert(gpu && SDL_GetError());
			assert(SDL_ClaimWindowForGPUDevice(gpu, app.window) && SDL_GetError());

			render_device.gpu = gpu;
		});

	world.system<Application, RenderDevice, WhiteTexture>()
		.kind(Phases::OnStart)
		.each([](Application& app, RenderDevice& render_device, WhiteTexture& white_texture) {
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

			white_texture.texture = texture;
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

	world.system<Application, RenderDevice, RenderCommands>()
		.kind(Phases::Clear)
		.each([](Application& app, RenderDevice& device, RenderCommands& render_commands) {
			render_commands.cmd_buffer = SDL_AcquireGPUCommandBuffer(device.gpu);

			assert(SDL_WaitAndAcquireGPUSwapchainTexture(render_commands.cmd_buffer, app.window, &render_commands.swapchain_texture, nullptr, nullptr) && SDL_GetError());

			auto color_target = SDL_GPUColorTargetInfo{
				.texture = render_commands.swapchain_texture,
				.clear_color = { 0.f, 0.f, 0.f, 1.f },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE
			};

			render_commands.render_pass = SDL_BeginGPURenderPass(render_commands.cmd_buffer, &color_target, 1, nullptr);

			SDL_PushGPUDebugGroup(render_commands.cmd_buffer, "render");
		});

	world.system<RenderCommands>()
		.kind(Phases::Display)
		.each([](RenderCommands& render_commands) {
			SDL_EndGPURenderPass(render_commands.render_pass);
			SDL_PopGPUDebugGroup(render_commands.cmd_buffer);

			assert(SDL_SubmitGPUCommandBuffer(render_commands.cmd_buffer) && SDL_GetError());
		});

	world.add<RenderDevice>();
	world.add<CopyCommands>();
	world.add<RenderCommands>();
	world.add<WhiteTexture>();
}
