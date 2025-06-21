#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/appModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ext/matrix_transform.hpp"
#include "utils/sdl.h"

using namespace ps;

SpriteModule::SpriteModule(flecs::world& world) {
	world.module<SpriteModule>();

	world.import<AppModule>();
	world.import<RenderModule>();
	world.import<TransformModule>();

	world.component<SpritePipeline>();
	world.component<Sprite>()
		.add(flecs::With, world.component<Mesh>())
		.add(flecs::With, world.component<ModelMatrix>())
		.add(flecs::With, world.component<Material>())
		.add(flecs::With, world.component<Transform>())
		.add(flecs::With, world.component<Dirty>());

	world.component<std::string>()
		.opaque(flecs::String)
			.serialize([](const flecs::serializer *s, const std::string *data) {
				const char *str = data->c_str();
				return s->value(flecs::String, &str);
			})
			.assign_string([](std::string* data, const char *value) {
				*data = value;
			});

	world.component<SDL_FColor>()
		.member<float>("r")
		.member<float>("g")
		.member<float>("b")
		.member<float>("a");

	world.component<Sprite>()
		.member<glm::vec2>("texture_size")
		.member<glm::vec2>("origin")
		.member<std::optional<glm::vec2>>("custom_size")
		.member<SDL_FColor>("color")
		.member<std::string>("path");

	world.observer<Sprite>()
		.event(flecs::OnSet)
		.each([](flecs::entity e, Sprite& s) {
			e.add<Dirty>();
		});

	world.system<Application, Renderer, SpritePipeline>("create sprite pipeline")
		.term_at(0).singleton()
		.term_at(1).singleton()
		.term_at(2).singleton()
		.kind(Phases::OnStart)
		.each([](Application& app, Renderer& r, SpritePipeline& sp) {
			auto vert_shader = load_shader(*r.gpu, "assets/shaders/out/shader.vert.msl", 1);
			auto frag_shader = load_shader(*r.gpu, "assets/shaders/out/shader.frag.msl", 0, 1);

			const auto white = glm::vec4{ 1, 1, 1, 1 };

			std::array<internal::VertexData, 4> verticies{
				internal::VertexData{ glm::vec3{ 0.f, 1.f, 0.f }, white, { 0.f, 1.f }},
				internal::VertexData{ glm::vec3{ 1.f, 1.f, 0.f }, white, { 1.f, 1.f }},
				internal::VertexData{ glm::vec3{ 1.f, 0.f, 0.f }, white, { 1.f, 0.f }},
				internal::VertexData{ glm::vec3{ 0.f, 0.f, 0.f }, white, { 0.f, 0.f }}
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

			auto vertex_buf = SDL_CreateGPUBuffer(r.gpu, &vertex_buffer_create_info);
			auto index_buf = SDL_CreateGPUBuffer(r.gpu, &index_buffer_create_info);

			SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info{
				.usage = SDL_GPUTransferBufferUsage::SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
				.size = static_cast<uint32_t>(verticies_byte_szie + indecies_byte_szie)
			};

			auto transfer_buf = SDL_CreateGPUTransferBuffer(r.gpu, &transfer_buffer_create_info);

			auto transfer_mem = SDL_MapGPUTransferBuffer(r.gpu ,transfer_buf, false);

			std::memcpy(transfer_mem, verticies.data(), verticies_byte_szie);
			std::memcpy(static_cast<char*>(transfer_mem) + verticies_byte_szie, indecies.data(), indecies_byte_szie);

			SDL_UnmapGPUTransferBuffer(r.gpu, transfer_buf);

			auto copy_cmd_buf = SDL_AcquireGPUCommandBuffer(r.gpu);

			auto copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

			SDL_GPUTransferBufferLocation vertex_buffer_location{ .transfer_buffer = transfer_buf };
			SDL_GPUTransferBufferLocation index_buffer_location{ .transfer_buffer = transfer_buf, .offset = static_cast<uint32_t>(verticies_byte_szie) };

			SDL_GPUBufferRegion vertex_buffer_region{ .buffer = vertex_buf, .size = static_cast<uint32_t>(verticies_byte_szie) };
			SDL_GPUBufferRegion index_buffer_region{ .buffer = index_buf, .size = static_cast<uint32_t>(indecies_byte_szie) };

			SDL_UploadToGPUBuffer(copy_pass,
				&vertex_buffer_location,
				&vertex_buffer_region,
				false
			);

			SDL_UploadToGPUBuffer(copy_pass,
				&index_buffer_location,
				&index_buffer_region,
				false
			);

			SDL_EndGPUCopyPass(copy_pass);

			auto result = SDL_SubmitGPUCommandBuffer(copy_cmd_buf);

			std::array<SDL_GPUVertexAttribute, 3> vertex_attrs{
				SDL_GPUVertexAttribute{
					.location = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = offsetof(internal::VertexData, position),
				},
				SDL_GPUVertexAttribute{
					.location = 1,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = offsetof(internal::VertexData, color),
				},
				SDL_GPUVertexAttribute{
					.location = 2,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
					.offset = offsetof(internal::VertexData, uv),
				},
			};

			SDL_GPUVertexBufferDescription vertex_buffer_description{
				.slot = 0,
				.pitch = sizeof(internal::VertexData),
			};

			SDL_GPUColorTargetDescription color_target_description{
				.format = SDL_GetGPUSwapchainTextureFormat(r.gpu, app.window)
			};

			SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info{
				.vertex_shader = vert_shader,
				.fragment_shader = frag_shader,
				.vertex_input_state = {
					.vertex_buffer_descriptions = &vertex_buffer_description,
					.num_vertex_buffers = 1,
					.vertex_attributes = vertex_attrs.data(),
					.num_vertex_attributes = vertex_attrs.size(),
				},
				.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
				.target_info = {
					.color_target_descriptions = &color_target_description,
					.num_color_targets = 1,
				}
			};

			auto pipeline = SDL_CreateGPUGraphicsPipeline(r.gpu, &pipeline_create_info);

			SDL_ReleaseGPUShader(r.gpu, vert_shader);
			SDL_ReleaseGPUShader(r.gpu, frag_shader);

			if (!pipeline) {
				print_sdl_error();

				return;
			}

			sp.pipeline = pipeline;
			sp.index_buffer = index_buf;
			sp.vertex_buffer = vertex_buf;

			SDL_GPUSamplerCreateInfo sampler_create_info{};
			sp.sampler = SDL_CreateGPUSampler(r.gpu, &sampler_create_info);
		});

	world.system<Renderer, SpritePipeline, AssetStorage, GlobalTransform, Mesh, Material, ModelMatrix, Sprite>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.term_at(2).singleton()
		.with<Dirty>()
		.kind(Phases::Update)
		.each([](Renderer& r, SpritePipeline& sp, AssetStorage& as, GlobalTransform& t, Mesh& m, Material& mat, ModelMatrix& mm, Sprite& s) {
			m.index_buffer = sp.index_buffer;
			m.vertex_buffer = sp.vertex_buffer;
			mat.pipeline = sp.pipeline;
			mat.sampler = sp.sampler;
			if (!s.path.empty()) {
				mat.texture = as.load_texture(*r.gpu, s.path);
				if (mat.texture) {
					s.texture_size = mat.texture->get_size();
				}
			} else {
				mat.texture = std::make_shared<Texture>(r.gpu, r.white_texture, s.custom_size ? s.custom_size.value() : s.texture_size);
			}
			mm.matrix = t.matrix * glm::scale(glm::mat4(1.f), glm::vec3(s.custom_size ? s.custom_size.value() : s.texture_size, 0.f));
		});


	world.system<Renderer, AssetStorage, SpritePipeline>()
		.term_at(0).singleton()
		.term_at(1).singleton()
		.kind(Phases::OnStart)
		.each([](flecs::iter& it, size_t, Renderer& r, AssetStorage& assets, SpritePipeline& sp) {
			it.world().entity("sprite").set<Sprite>({ .path = "assets/main_menu.jpg" });
			it.world().entity("sprite_default").add<Sprite>();
		});

	world.add<SpritePipeline>();
}
