#pragma once

#include "SDL3/SDL.h"
#include "vec2.hpp"

namespace ps {
	class Texture {
	public:
		Texture(SDL_GPUDevice* gpu, SDL_GPUTexture* texture, const glm::vec2& size);
		~Texture();

		SDL_GPUTexture& get_gpu_texture() const;
		const glm::vec2& get_size() const;
	private:
		SDL_GPUTexture* m_texture;
		SDL_GPUDevice* m_gpu;
		glm::vec2 m_size;
	};
}
