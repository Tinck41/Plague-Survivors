#pragma once

#include "sdl.h"

namespace ps {
	//template<typename T>
	//class GpuBuffer {
	//public:
	//	GpuBuffer(SDL_GPUDevice* gpu, SDL_GPUBufferUsageFlags usage, size_t capacity) {
	//		GpuBuffer buffer;

	//		buffer.capacity = capacity;

	//		SDL_GPUBufferCreateInfo vertex_buffer_create_info{
	//			.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
	//			.size = sizeof(T) * capacity,
	//		};

	//		buffer.handle = SDL_CreateGPUBuffer(gpu, &vertex_buffer_create_info);

	//		return buffer;
	//	}

	//private:
	//	SDL_GPUBuffer* handle;
	//	size_t capacity;
	//};
}
