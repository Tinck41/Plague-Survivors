#include "module.h"

#include "SDL3_image/SDL_image.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/common.h"

using namespace ps;

void AssetStorage::update() {
	auto it = textures.begin();

	while (it != textures.end()) {
		if (it->second.use_count() == 0) {
			it = textures.erase(it);
		} else {
			++it;
		}
	}
}

std::shared_ptr<Texture> AssetStorage::load_texture(SDL_GPUDevice& gpu, const std::string& path) {
	if (path.empty()) {
		return nullptr;
	}

	if (!textures.contains(path)) {
		SDL_Surface* img = IMG_Load(path.c_str());
		SDL_Surface* image = SDL_ConvertSurface(img, SDL_PIXELFORMAT_RGBA32);
		SDL_GPUTextureCreateInfo texture_create_info{
			.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
			.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
			.width = static_cast<uint32_t>(image->w),
			.height = static_cast<uint32_t>(image->h),
			.layer_count_or_depth = 1,
			.num_levels = 1,
		};

		SDL_GPUTexture* texture = SDL_CreateGPUTexture(&gpu, &texture_create_info);

		const auto image_byte_size = image->w * image->h * 4;

		SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = static_cast<uint32_t>(image_byte_size),
		};
		auto tex_transfer_buf = SDL_CreateGPUTransferBuffer(&gpu, &transfer_buffer_create_info);

		auto tex_transfer_mem = SDL_MapGPUTransferBuffer(&gpu, tex_transfer_buf, false);

		std::memcpy(tex_transfer_mem, image->pixels, image_byte_size);

		SDL_UnmapGPUTransferBuffer(&gpu, tex_transfer_buf);

		auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(&gpu);
		auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

		SDL_GPUTextureTransferInfo texture_transfer_info{
			.transfer_buffer = tex_transfer_buf,
		};
		SDL_GPUTextureRegion texture_region{
			.texture = texture,
			.w = static_cast<uint32_t>(image->w),
			.h = static_cast<uint32_t>(image->h),
			.d = 1,
		};

		SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);

		SDL_EndGPUCopyPass(copy_pass);
		auto result = SDL_SubmitGPUCommandBuffer(copy_cmd_buf); assert(result);

		textures[path] = std::make_shared<Texture>(&gpu, texture, glm::vec2{ image->w, image->h });

		SDL_ReleaseGPUTransferBuffer(&gpu, tex_transfer_buf);
		SDL_DestroySurface(image);
		SDL_DestroySurface(img);
	}

	return textures.at(path);
}

std::shared_ptr<Font> AssetStorage::load_font(const std::string& path, float size) {
	if (!fonts.contains(path)) {
		TTF_Font* resource = TTF_OpenFont(path.c_str(), size);

		TTF_SetFontSDF(resource, true);
		TTF_SetFontWrapAlignment(resource, TTF_HORIZONTAL_ALIGN_CENTER);

		fonts[path] = std::make_shared<Font>(resource);
	}

	return fonts.at(path);
}

AssetModule::AssetModule(flecs::world& world) {
	world.module<AssetModule>();

	world.import<RenderModule>();

	world.component<AssetStorage>().add(flecs::Singleton) ;

	world.system<AssetStorage>()
		.kind(Phases::Update)
		.each([](AssetStorage& storage) {
			storage.update();
		});

	world.add<AssetStorage>();
}

