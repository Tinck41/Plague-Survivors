#pragma once

#include "SDL3/SDL.h"
#include "color.h"
#include "vec2.hpp"

namespace ps {
	class Texture {
	public:
		Texture(SDL_GPUDevice* gpu, SDL_GPUTexture* texture, const glm::vec2& size);
		Texture(SDL_GPUDevice* gpu, glm::uvec2 size, Color color = WHITE, SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
		~Texture();

		SDL_GPUTexture& get_gpu_texture() const;
		const glm::vec2& get_size() const;
	private:
		SDL_GPUTexture* m_texture;
		SDL_GPUDevice* m_gpu;
		glm::vec2 m_size;
	};
}
