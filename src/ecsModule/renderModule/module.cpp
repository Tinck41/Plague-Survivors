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

	world.component<Renderer>().add(flecs::Singleton);
	world.component<RenderPass>().add(flecs::Singleton);
	world.component<RenderDevice>().add(flecs::Singleton);
	world.component<WhiteTexture>().add(flecs::Singleton);
	world.component<ModelMatrix>();

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

	world.system<Application, RenderDevice, RenderPass>()
		.kind(Phases::PreRender)
		.each([](Application& app, RenderDevice& device, RenderPass& render_pass) {
			render_pass.cmd_buffer = SDL_AcquireGPUCommandBuffer(device.gpu);

			assert(SDL_WaitAndAcquireGPUSwapchainTexture(render_pass.cmd_buffer, app.window, &render_pass.swapchain_texture, nullptr, nullptr) && SDL_GetError());

			auto color_target = SDL_GPUColorTargetInfo{
				.texture = render_pass.swapchain_texture,
				.clear_color = { 0.f, 0.f, 0.f, 1.f },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE
			};

			render_pass.render_pass = SDL_BeginGPURenderPass(render_pass.cmd_buffer, &color_target, 1, nullptr);

			SDL_PushGPUDebugGroup(render_pass.cmd_buffer, "render");
		});

	world.system<RenderPass>()
		.kind(Phases::PostRender)
		.each([](RenderPass& render_pass) {
			SDL_EndGPURenderPass(render_pass.render_pass);

			SDL_PopGPUDebugGroup(render_pass.cmd_buffer);
			assert(SDL_SubmitGPUCommandBuffer(render_pass.cmd_buffer) && SDL_GetError());

		});

	//world.system<const Mesh, const Material>("render")
	//	.kind(Phases::Render)
	//	.run([](flecs::iter& it) {
	//		const auto& app = it.world().get<Application>();
	//		const auto& renderer = it.world().get<Renderer>();
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
	//		auto render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, nullptr);

	//		while(it.next()) {
	//			const auto meshes = it.field<const Mesh>(0);
	//			const auto materials = it.field<const Material>(1);

	//			for (auto i : it) {
	//				internal::UBO ubo{ 
	//					.view_proj = projection * view
	//				};

	//				SDL_GPUBufferBinding vertex_buffer_binding{
	//					.buffer = meshes[i].vertex_buffer,
	//				};
	//				SDL_GPUBufferBinding index_buffer_binding{
	//					.buffer = meshes[i].index_buffer,
	//				};

	//				SDL_BindGPUGraphicsPipeline(render_pass, materials[i].pipeline);
	//				SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
	//				SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
	//				SDL_PushGPUVertexUniformData(cmd_buf, 0, &ubo, sizeof(ubo));

	//				SDL_GPUTextureSamplerBinding texture_sampler_binding{
	//					.texture = &materials[i].texture->get_gpu_texture(),
	//					.sampler = materials[i].sampler,
	//				};

	//				SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);

	//				SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
	//			}
	//		}

	//		SDL_EndGPURenderPass(render_pass);

	//		result &= SDL_SubmitGPUCommandBuffer(cmd_buf); assert(result);
	//	});

	world.add<RenderDevice>();
	world.add<RenderPass>();
	world.add<WhiteTexture>();
}
