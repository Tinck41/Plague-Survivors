#pragma once

#include "flecs.h"
#include <functional>
#include <unordered_map>

namespace ps {
	struct Tween {};
	struct Single {};
	struct Sequence {};
	struct Simultaneous {};
	struct Loop {};

	struct TweenFrom {};
	struct TweenTo {};

	struct TweenTarget {};

	struct TweenDuration {
		float value = 0.f;
	};

	struct TweenTimer {
		float value = 0.f;
	};

	struct TweenFunction {};

	struct TweenState {};
	struct TweenNextState {};
	struct Waiting {};
	struct Running {};
	struct Completed {};

	namespace easing {
		struct Linear {};
		struct QuadraticEaseIn {};
		struct QuadraticEaseOut {};
		struct QuadraticEaseInOut {};
		struct CubicEaseIn {};
		struct CubicEaseOut {};
		struct CubicEaseInOut {};
		struct QuarticEaseIn {};
		struct QuarticEaseOut {};
		struct QuarticEaseInOut {};
		struct QuinticEaseIn {};
		struct QuinticEaseOut {};
		struct QuinticEaseInOut {};
		struct SineEaseIn {};
		struct SineEaseOut {};
		struct SineEaseInOut {};
		struct CircularEaseIn {};
		struct CircularEaseOut {};
		struct CircularEaseInOut {};
		struct ExponentialEaseIn {};
		struct ExponentialEaseOut {};
		struct ExponentialEaseInOut {};
		struct BackEaseIn {};
		struct BackEaseOut {};
		struct BackEaseInOut {};
		struct BounceEaseIn {};
		struct BounceEaseOut {};
		struct BounceEaseInOut {};
	}

	struct TweenModule {
		TweenModule(flecs::world world);

		inline static std::unordered_map<flecs::entity_t, std::function<float(float)>> tweenFunctions;
		inline static std::unordered_map<flecs::entity_t, std::function<void(flecs::entity&)>> containerFunctions;
	};
}
