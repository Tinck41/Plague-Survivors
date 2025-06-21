#include "texture.h"

using namespace ps;

Texture::Texture(SDL_GPUDevice* gpu, SDL_GPUTexture* texture, const glm::vec2& size) : m_gpu(gpu), m_texture(texture), m_size(size) {}

Texture::~Texture() {
	SDL_ReleaseGPUTexture(m_gpu, m_texture);
}

SDL_GPUTexture& Texture::get_gpu_texture() const {
	return *m_texture;
}

const glm::vec2& Texture::get_size() const {
	return m_size;
}

