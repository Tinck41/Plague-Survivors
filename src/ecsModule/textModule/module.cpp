#include "module.h"

#include "ecsModule/appModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/common.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "font.h"

using namespace ps;

static std::map<flecs::entity_t, std::shared_ptr<Texture>> font_textures;

TextModule::TextModule(flecs::world& world) {
	world.module<TextModule>();

	world.import<RenderModule>();
	world.import<SpriteModule>();
	world.import<TransformModule>();

	world.component<Text>()
		.add(flecs::With, world.component<Transform>());

	world.system<Text, RenderDevice>()
		.kind(Phases::OnStart)
		.each([world](flecs::entity e, Text& text, RenderDevice& device) {
			SDL_Surface* surf = TTF_RenderText_Blended(*text.font, text.string.c_str(), text.string.size(), SDL_Color{ 255, 255, 255, 255 });
			SDL_Surface* surface = SDL_ConvertSurface(surf, SDL_PIXELFORMAT_RGBA32);

			SDL_GPUTextureCreateInfo texture_create_info{
				.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
				.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
				.width = static_cast<uint32_t>(surface->w),
				.height = static_cast<uint32_t>(surface->h),
				.layer_count_or_depth = 1,
				.num_levels = 1,
			};

			SDL_GPUTexture* texture = SDL_CreateGPUTexture(device.gpu, &texture_create_info);

			const auto surface_byte_size = surface->w * surface->h * 4;

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<uint32_t>(surface_byte_size),
			};
			auto tex_transfer_buf = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

			auto tex_transfer_mem = SDL_MapGPUTransferBuffer(device.gpu, tex_transfer_buf, false);

			std::memcpy(tex_transfer_mem, surface->pixels, surface_byte_size);

			SDL_UnmapGPUTransferBuffer(device.gpu, tex_transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(device.gpu);
			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

			SDL_GPUTextureTransferInfo texture_transfer_info{
				.transfer_buffer = tex_transfer_buf,
			};
			SDL_GPUTextureRegion texture_region{
				.texture = texture,
				.w = static_cast<uint32_t>(surface->w),
				.h = static_cast<uint32_t>(surface->h),
				.d = 1,
			};

			SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);

			SDL_EndGPUCopyPass(copy_pass);
			auto result = SDL_SubmitGPUCommandBuffer(copy_cmd_buf); assert(result);

			font_textures[e] = std::make_shared<Texture>(device.gpu, texture, glm::vec2{ surface->w, surface->h });

			SDL_ReleaseGPUTransferBuffer(device.gpu, tex_transfer_buf);
			SDL_DestroySurface(surface);
		});

	world.system<const Text, const GlobalTransform, SpritesRenderData>()
		.kind(Phases::PostUpdate)
		.each([](flecs::entity e, const Text& text, const GlobalTransform& transform, SpritesRenderData& sprites_render_data) {
			sprites_render_data.emplace_back(e, font_textures.at(e), std::nullopt, transform, text.color);
		});

	TTF_Init();

	world.entity().set<Text>({ .string = "hello SDL3_ttf!", .font = world.get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf", 46) });
}
