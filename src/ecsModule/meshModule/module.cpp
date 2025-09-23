#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/renderModule/module.h"

using namespace ps;

MeshModule::MeshModule(flecs::world& world) {
	world.module<MeshModule>();

	world.import<RenderModule>();

	//world.component<Mesh>();
	world.component<Material>();
	world.component<QuadMesh>().add(flecs::Singleton);

	world.system<RenderDevice, QuadMesh>()
		.kind(Phases::OnStart)
		.each([](RenderDevice& device, QuadMesh& quad) {
			constexpr auto white = glm::vec4{ 1, 1, 1, 1 };

			std::array<Vertex, 4> verticies{
				Vertex{ glm::vec3{ 0.f, 1.f, 0.f }, white, { 0.f, 1.f }},
				Vertex{ glm::vec3{ 1.f, 1.f, 0.f }, white, { 1.f, 1.f }},
				Vertex{ glm::vec3{ 1.f, 0.f, 0.f }, white, { 1.f, 0.f }},
				Vertex{ glm::vec3{ 0.f, 0.f, 0.f }, white, { 0.f, 0.f }}
			};

			std::array<uint16_t, 6> indecies{
				0, 1, 2,
				2, 3, 0
			};

			const auto verticies_byte_szie = verticies.size() * sizeof(verticies[0]);
			const auto indecies_byte_szie = indecies.size() * sizeof(indecies[0]);

			SDL_GPUBufferCreateInfo vertex_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
				.size = verticies_byte_szie,
			};
			SDL_GPUBufferCreateInfo index_buffer_create_info{
				.usage = SDL_GPU_BUFFERUSAGE_INDEX,
				.size = indecies_byte_szie,
			};

			auto vertex_buf = SDL_CreateGPUBuffer(device.gpu, &vertex_buffer_create_info);
			auto index_buf = SDL_CreateGPUBuffer(device.gpu, &index_buffer_create_info);

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPUTransferBufferUsage::SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<uint32_t>(verticies_byte_szie + indecies_byte_szie)
			};

			auto transfer_buf = SDL_CreateGPUTransferBuffer(device.gpu, &transfer_buffer_create_info);

			auto transfer_mem = SDL_MapGPUTransferBuffer(device.gpu ,transfer_buf, false);

			std::memcpy(transfer_mem, verticies.data(), verticies_byte_szie);
			std::memcpy(static_cast<char*>(transfer_mem) + verticies_byte_szie, indecies.data(), indecies_byte_szie);

			SDL_UnmapGPUTransferBuffer(device.gpu, transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(device.gpu);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

			SDL_GPUTransferBufferLocation vertex_buffer_location{ .transfer_buffer = transfer_buf };
			SDL_GPUTransferBufferLocation index_buffer_location{ .transfer_buffer = transfer_buf, .offset = static_cast<uint32_t>(verticies_byte_szie) };

			SDL_GPUBufferRegion vertex_buffer_region{ .buffer = vertex_buf, .size = static_cast<uint32_t>(verticies_byte_szie) };
			SDL_GPUBufferRegion index_buffer_region{ .buffer = index_buf, .size = static_cast<uint32_t>(indecies_byte_szie) };

			SDL_UploadToGPUBuffer(copy_pass, &vertex_buffer_location, &vertex_buffer_region, false);
			SDL_UploadToGPUBuffer(copy_pass, &index_buffer_location, &index_buffer_region, false);

			SDL_EndGPUCopyPass(copy_pass);

			assert(SDL_SubmitGPUCommandBuffer(copy_cmd_buf) && SDL_GetError());

			quad.index_buffer = index_buf;
			quad.vertex_buffer = vertex_buf;
		});

	world.set(QuadMesh{});
}
