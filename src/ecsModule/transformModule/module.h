#pragma once

#include "SFML/System/Vector2.hpp"
#include "SFML/System/Vector3.hpp"
#include "flecs.h"

namespace ps {
	struct Dirty {};

	struct Vec2f : public sf::Vector2f {
		Vec2f() = default;
		Vec2f(float x, float y) : sf::Vector2f(x,y) {}
	};

	struct Vec3f : public sf::Vector3f {
		Vec3f() = default;
		Vec3f(float x, float y) : sf::Vector3f(x, y, 0.f) {}
		Vec3f(float x, float y, float z) : sf::Vector3f(x, y, z) {}

		operator Vec2f () { return Vec2f{ x, y }; }
	};

	struct Transform {
		Vec3f translation{ 0.f, 0.f, 0.f };
		Vec3f scale{ 1.f, 1.f, 1.f };
		float rotation = 0.f;
	};

	struct GlobalTransform : public Transform {};

	struct TransformModule {
		TransformModule(flecs::world& world);
	};
}
