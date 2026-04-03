#pragma once

#include "color.h"
#include "dag.h"
#include "flecs.h"
#include "vec2.hpp"
#include "mat4x4.hpp"
#include "texture.h"

#include <unordered_set>
#include <variant>
#include <memory>

namespace ps {
	using CameraCompositionGraph = Dag<flecs::entity>;

	struct Camera {
		glm::vec2 viewport;
		glm::vec2 offset;
		glm::mat4 projection;

		std::variant<std::monostate, flecs::entity_t, std::shared_ptr<Texture>> render_target;

		Color clear_color = TRANSPARENT;

		SDL_GPULoadOp load_op = SDL_GPU_LOADOP_CLEAR;

		bool skip_visible_check = false;
	};

	struct WindowResize {
		flecs::entity_t window_entity;

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

	struct RenderLayers {
		std::uint32_t mask = 1u << 1;

		static std::uint32_t layer(std::uint32_t layer) {
			return 1u << layer;
		};

		RenderLayers operator|(const RenderLayers& other) const {
			return { mask | other.mask };
		}

		bool intersects(const RenderLayers& other) const {
			return (mask & other.mask) != 0;
		}

		constexpr static std::uint32_t DEFAULT = 1u << 1;
		constexpr static std::uint32_t UI = 1u << 2;
	};

	struct CameraModule {
		CameraModule(flecs::world& world);
	};
}
