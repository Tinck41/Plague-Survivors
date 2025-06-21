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

	world.component<Renderer>();
	world.component<Mesh>();
	world.component<Material>();
	world.component<ModelMatrix>();

	world.component<SDL_Color>()
		.member<unsigned char>("r")
		.member<unsigned char>("g")
		.member<unsigned char>("b")
		.member<unsigned char>("a");

	world.system<Application, Renderer>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(Phases::OnStart)
		.each([](Application& app, Renderer& renderer) {
			auto gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, true, NULL);

			if (!gpu) {
				print_sdl_error();

				return;
			}

			auto result = SDL_ClaimWindowForGPUDevice(gpu, app.window); //assert(result);

			if (!result) {
				print_sdl_error();

				return;
			}

			auto texture_create_info = SDL_GPUTextureCreateInfo{
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
				.width = 1,
				.height = 1,
				.layer_count_or_depth = 1,
				.num_levels = 1,
			};
			auto white_texture = SDL_CreateGPUTexture(gpu, &texture_create_info);

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = 4,
			};
			auto tex_transfer_buf = SDL_CreateGPUTransferBuffer(gpu, &transfer_buffer_create_info);
			auto tex_transfer_mem = SDL_MapGPUTransferBuffer(gpu, tex_transfer_buf, false);

			uint32_t white_pixel = 0xffffffff;
			std::memcpy(tex_transfer_mem, &white_pixel, 4);

			SDL_UnmapGPUTransferBuffer(gpu, tex_transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(gpu);
			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

			SDL_GPUTextureTransferInfo texture_transfer_info{
				.transfer_buffer = tex_transfer_buf,
			};
			SDL_GPUTextureRegion texture_region{
				.texture = white_texture,
				.w = 1,
				.h = 1,
				.d = 1,
			};

			SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);

			SDL_EndGPUCopyPass(copy_pass);
			result = SDL_SubmitGPUCommandBuffer(copy_cmd_buf); assert(result);

			SDL_ReleaseGPUTransferBuffer(gpu, tex_transfer_buf);

			renderer.gpu = gpu;
			renderer.white_texture = white_texture;
		});


	world.system<const Mesh, const Material, const ModelMatrix>("render")
		.kind(Phases::Render)
		.run([](flecs::iter& it) {
			const auto app = it.world().get<Application>();
			const auto renderer = it.world().get<Renderer>();
			const auto camera = it.world().entity(CameraModule::EcsCamera);
			const auto view = glm::translate(glm::mat4(1.f), -camera.get<GlobalTransform>()->translation);
			const auto projection = camera.get<Camera>()->projection;

			auto cmd_buf = SDL_AcquireGPUCommandBuffer(renderer->gpu);

			SDL_GPUTexture* swapchain_texture;

			auto result = SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, app->window, &swapchain_texture, nullptr, nullptr); assert(result);

			auto color_target = SDL_GPUColorTargetInfo{
				.texture = swapchain_texture,
				.clear_color = { 0.f, 0.f, 0.f, 1.f },
				.load_op = SDL_GPU_LOADOP_CLEAR,
				.store_op = SDL_GPU_STOREOP_STORE
			};
			auto render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, nullptr);

			while(it.next()) {
				const auto meshes = it.field<const Mesh>(0);
				const auto materials = it.field<const Material>(1);
				const auto model = it.field<const ModelMatrix>(2); // TODO: doesn't update with transform

				for (auto i : it) {
					internal::UBO ubo{ 
						.mvp = projection * view * model[i].matrix
					};

					SDL_GPUBufferBinding vertex_buffer_binding{
						.buffer = meshes[i].vertex_buffer,
					};
					SDL_GPUBufferBinding index_buffer_binding{
						.buffer = meshes[i].index_buffer,
					};

					SDL_BindGPUGraphicsPipeline(render_pass, materials[i].pipeline);
					SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
					SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
					SDL_PushGPUVertexUniformData(cmd_buf, 0, &ubo, sizeof(ubo));
					if (materials[i].sampler && materials[i].texture) {
						SDL_GPUTextureSamplerBinding texture_sampler_binding{
							.texture = &materials[i].texture->get_gpu_texture(),
							.sampler = materials[i].sampler,
						};
						SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
					} else {
						SDL_GPUTextureSamplerBinding texture_sampler_binding{
							.texture = renderer->white_texture,
							.sampler = materials[i].sampler,
						};
						SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
					}

					SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
				}
			}

			SDL_EndGPURenderPass(render_pass);

			result &= SDL_SubmitGPUCommandBuffer(cmd_buf); assert(result);
		});

	world.add<Renderer>();
}
