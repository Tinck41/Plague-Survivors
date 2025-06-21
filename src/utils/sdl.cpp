#include "sdl.h"

#include "spdlog/spdlog.h"
#include "SDL3/SDL.h"

#include <fstream>

void ps::print_sdl_error() {
	spdlog::error("{}", SDL_GetError());
}


SDL_GPUShader* ps::load_shader(SDL_GPUDevice& gpu, std::string_view path, uint32_t num_uniform_buf, uint32_t num_samplers) {
	std::ifstream file;
	SDL_GPUShaderStage stage;

	if (path.contains("vert")) {
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	} else if (path.contains("frag")) {
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	} else {
		spdlog::error("unsupported shader format for {}", path);
	}

	file.open(path, std::ios::binary | std::ios::in);
	if (!file.is_open()) {
		spdlog::error("Failed to open shader file: {}", path);
		return nullptr;
	}
	std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(file), {});
	file.close();

	SDL_GPUShaderCreateInfo shader_create_info{
		.code_size = buffer.size(),
		.code = buffer.data(),
		.entrypoint = "main0",
		.format = SDL_GPU_SHADERFORMAT_MSL,
		.stage = stage,
		.num_samplers = num_samplers,
		.num_uniform_buffers = num_uniform_buf,
	};

	return SDL_CreateGPUShader(&gpu, &shader_create_info);
}
