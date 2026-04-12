#include "texture.h"

using namespace ps;

Texture::Texture(SDL_GPUDevice* gpu, SDL_GPUTexture* texture, const glm::vec2& size) : m_gpu(gpu), m_texture(texture), m_size(size) {}

Texture::Texture(SDL_GPUDevice* gpu, glm::uvec2 size, Color color, SDL_GPUTextureFormat format) {
	auto texture_create_info = SDL_GPUTextureCreateInfo{
		.format = format,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.width = size.x,
		.height = size.y,
		.layer_count_or_depth = 1,
		.num_levels = 1,
	};

	auto texture = SDL_CreateGPUTexture(gpu, &texture_create_info);

	const auto pixel_count = size.x * size.y;
	const auto buffer_size = pixel_count * 4;

	SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = buffer_size,
	};
	auto tex_transfer_buf = SDL_CreateGPUTransferBuffer(gpu, &transfer_buffer_create_info);
	auto tex_transfer_mem = SDL_MapGPUTransferBuffer(gpu, tex_transfer_buf, false);

	for (uint32_t i = 0; i < pixel_count; i++) {
		std::memcpy(static_cast<uint8_t*>(tex_transfer_mem) + i * 4, &color, 4);
	}

	SDL_UnmapGPUTransferBuffer(gpu, tex_transfer_buf);

	auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(gpu);
	auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

	SDL_GPUTextureTransferInfo texture_transfer_info{
		.transfer_buffer = tex_transfer_buf,
	};
	SDL_GPUTextureRegion texture_region{
		.texture = texture,
		.w = size.x,
		.h = size.y,
		.d = 1,
	};

	SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);

	SDL_EndGPUCopyPass(copy_pass);

	assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());

	SDL_ReleaseGPUTransferBuffer(gpu, tex_transfer_buf);

	m_texture = texture;
	m_size = size;
}

Texture::~Texture() {
	SDL_ReleaseGPUTexture(m_gpu, m_texture);
}

SDL_GPUTexture& Texture::get_gpu_texture() const {
	return *m_texture;
}

const glm::vec2& Texture::get_size() const {
	return m_size;
}

