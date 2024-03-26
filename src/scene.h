#pragma once

#include "flecs.h"
#include <memory>

namespace ps::core {
	class Scene {
	public:
		[[nodiscard]] static std::unique_ptr<Scene> create();

		flecs::entity getRootEntity();
	private:
		Scene() = default;

		flecs::entity m_root;
	};
}
