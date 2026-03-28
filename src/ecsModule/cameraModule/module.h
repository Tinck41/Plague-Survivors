#pragma once

#include "color.h"
#include "flecs.h"
#include "vec2.hpp"
#include "mat4x4.hpp"
#include "texture.h"

#include <unordered_set>
#include <vector>
#include <memory>

namespace ps {
	struct Camera {
		glm::vec2 viewport;
		glm::vec2 offset;
		glm::mat4 projection;

		std::shared_ptr<Texture> render_texture;

		Color clear_color = BLACK;

		SDL_GPULoadOp load_op = SDL_GPU_LOADOP_CLEAR;
	};

	struct WindowResize {
		int width;
		int height;
	};

	struct Visible2d {
		bool value = false;
	};

	struct VisibleEntities {
		std::unordered_set<flecs::entity_t> entities;
	};

	struct Aabb {
		glm::vec2 min;
		glm::vec2 max;

		bool is_intersect(const Aabb& other) {
			if (other.min == other.max) {
				return false;
			}

			return 
				other.max.x > min.x &&
				other.min.x < max.x &&
				other.max.y > min.y &&
				other.min.y < max.y;
		}
	};

	struct CameraModule {
		CameraModule(flecs::world& world);
	};
}
