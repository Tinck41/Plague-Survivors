#pragma once

namespace ps::ecsModule {
	struct RigidbodyComponent {
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;
		bool fixedRotation = true;

		void* body = nullptr;
	};
}
