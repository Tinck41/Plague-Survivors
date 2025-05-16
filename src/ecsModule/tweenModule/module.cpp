#include "module.h"
#include "ecsModule/common.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/utils.h"

#include "gtx/easing.hpp"
#include "spdlog/spdlog.h"

using namespace ps;

TweenModule::TweenModule(flecs::world world) {
	world.module<TweenModule>();

	world.import<TransformModule>();
	world.import<TimeModule>();

	world.component<Tween>();
	world.component<Single>();
	world.component<Sequence>();
	world.component<Simultaneous>();
	world.component<Loop>();
	world.component<TweenFrom>();
	world.component<TweenTo>();
	world.component<TweenFunction>().add(flecs::DontFragment);
	world.component<easing::Linear>();
	world.component<easing::QuadraticEaseIn>();
	world.component<easing::QuadraticEaseOut>();
	world.component<easing::QuadraticEaseInOut>();
	world.component<easing::CubicEaseIn>();
	world.component<easing::CubicEaseOut>();
	world.component<easing::CubicEaseInOut>();
	world.component<easing::QuarticEaseIn>();
	world.component<easing::QuarticEaseOut>();
	world.component<easing::QuarticEaseInOut>();
	world.component<easing::QuinticEaseIn>();
	world.component<easing::QuinticEaseOut>();
	world.component<easing::QuinticEaseInOut>();
	world.component<easing::SineEaseIn>();
	world.component<easing::SineEaseOut>();
	world.component<easing::SineEaseInOut>();
	world.component<easing::CircularEaseIn>();
	world.component<easing::CircularEaseOut>();
	world.component<easing::CircularEaseInOut>();
	world.component<easing::ExponentialEaseIn>();
	world.component<easing::ExponentialEaseOut>();
	world.component<easing::ExponentialEaseInOut>();
	world.component<easing::BackEaseIn>();
	world.component<easing::BackEaseOut>();
	world.component<easing::BackEaseInOut>();
	world.component<easing::BounceEaseIn>();
	world.component<easing::BounceEaseOut>();
	world.component<easing::BounceEaseInOut>();
	world.component<TweenState>().add(flecs::Union);
	world.component<TweenNextState>().add(flecs::Union);
	world.component<Waiting>();
	world.component<Running>();
	world.component<Completed>();
	world.component<TweenTarget>();
	world.component<TweenTimer>();
	world.component<TweenDuration>().add(flecs::With, world.component<TweenTimer>());

	world.component<TweenTimer>()
		.member<float>("value");

	world.component<TweenDuration>()
		.member<float>("value");

	tweenFunctions[world.id<easing::Linear>()]               = [](float value) { return glm::linearInterpolation(value); };
	tweenFunctions[world.id<easing::QuadraticEaseIn>()]      = [](float value) { return glm::quadraticEaseIn(value); };
	tweenFunctions[world.id<easing::QuadraticEaseOut>()]     = [](float value) { return glm::quadraticEaseOut(value); };
	tweenFunctions[world.id<easing::QuadraticEaseInOut>()]   = [](float value) { return glm::quadraticEaseInOut(value); };
	tweenFunctions[world.id<easing::CubicEaseIn>()]          = [](float value) { return glm::cubicEaseIn(value); };
	tweenFunctions[world.id<easing::CubicEaseOut>()]         = [](float value) { return glm::cubicEaseOut(value); };
	tweenFunctions[world.id<easing::CubicEaseInOut>()]       = [](float value) { return glm::cubicEaseInOut(value); };
	tweenFunctions[world.id<easing::QuarticEaseIn>()]        = [](float value) { return glm::quarticEaseIn(value); };
	tweenFunctions[world.id<easing::QuarticEaseOut>()]       = [](float value) { return glm::quarticEaseOut(value); };
	tweenFunctions[world.id<easing::QuarticEaseInOut>()]     = [](float value) { return glm::quarticEaseInOut(value); };
	tweenFunctions[world.id<easing::QuinticEaseIn>()]        = [](float value) { return glm::quinticEaseIn(value); };
	tweenFunctions[world.id<easing::QuinticEaseOut>()]       = [](float value) { return glm::quinticEaseOut(value); };
	tweenFunctions[world.id<easing::QuinticEaseInOut>()]     = [](float value) { return glm::quinticEaseInOut(value); };
	tweenFunctions[world.id<easing::SineEaseIn>()]           = [](float value) { return glm::sineEaseIn(value); };
	tweenFunctions[world.id<easing::SineEaseOut>()]          = [](float value) { return glm::sineEaseOut(value); };
	tweenFunctions[world.id<easing::SineEaseInOut>()]        = [](float value) { return glm::sineEaseInOut(value); };
	tweenFunctions[world.id<easing::CircularEaseIn>()]       = [](float value) { return glm::circularEaseIn(value); };
	tweenFunctions[world.id<easing::CircularEaseOut>()]      = [](float value) { return glm::circularEaseOut(value); };
	tweenFunctions[world.id<easing::CircularEaseInOut>()]    = [](float value) { return glm::circularEaseInOut(value); };
	tweenFunctions[world.id<easing::ExponentialEaseIn>()]    = [](float value) { return glm::exponentialEaseIn(value); };
	tweenFunctions[world.id<easing::ExponentialEaseOut>()]   = [](float value) { return glm::exponentialEaseOut(value); };
	tweenFunctions[world.id<easing::ExponentialEaseInOut>()] = [](float value) { return glm::exponentialEaseInOut(value); };
	tweenFunctions[world.id<easing::BackEaseIn>()]           = [](float value) { return glm::backEaseIn(value); };
	tweenFunctions[world.id<easing::BackEaseOut>()]          = [](float value) { return glm::backEaseOut(value); };
	tweenFunctions[world.id<easing::BackEaseInOut>()]        = [](float value) { return glm::backEaseInOut(value); };
	tweenFunctions[world.id<easing::BounceEaseIn>()]         = [](float value) { return glm::bounceEaseIn(value); };
	tweenFunctions[world.id<easing::BounceEaseOut>()]        = [](float value) { return glm::bounceEaseOut(value); };
	tweenFunctions[world.id<easing::BounceEaseInOut>()]      = [](float value) { return glm::bounceEaseInOut(value); };

	using FromTransform = flecs::pair<TweenFrom, Transform>;
	using ToTransform   = flecs::pair<TweenTo, Transform>;

	using TweenSequence    = flecs::pair<Tween, Sequence>;
	using TweenSimultaneus = flecs::pair<Tween, Simultaneous>;
	using TweenSingle      = flecs::pair<Tween, Single>;
	using TweenLoop        = flecs::pair<Tween, Loop>;

	world.observer<Tween>("init tween state")
		.term_at(0).second(flecs::Wildcard)
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Tween){
			e.add(flecs::OrderedChildren);
			e.add<TweenState, Waiting>();
		});

	world.system("set tween target")
		.with<Tween>(flecs::Wildcard)
		.with(flecs::ChildOf).second(flecs::Wildcard)
		.without<TweenTarget>(flecs::Wildcard)
		.without<Tween>(flecs::Wildcard).parent()
		.without(flecs::Module).parent()
		.without(flecs::World).parent()
		.kind(Phases::Update)
		.each([](flecs::entity e) {
			const auto parent = e.parent();

			utils::dfs(e, [parent](flecs::entity child) {
				child.add<TweenTarget>(parent);
			});
		});

	world.system("execute root tween")
		.with<Tween>(flecs::Wildcard)
		.without<TweenState, Running>()
		.without<Tween>(flecs::Wildcard).parent()
		.without(flecs::Module).parent()
		.without(flecs::World).parent()
		.each([](flecs::entity e) {
			e.add<TweenState, Running>();
		});

	world.system<TweenNextState>("process tween next state")
		.term_at(0).second<Running>()
		.with<Tween>(flecs::Wildcard)
		.with<TweenNextState, Running>()
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, TweenNextState) {
			auto e = it.entity(i);

			e.remove<TweenNextState, Running>();
			e.add<TweenState, Running>();

			e.children([](flecs::entity e) {
				e.add<TweenState, Waiting>();
			});
		});

	world.system<TweenSequence>("process tween sequence")
		.with<TweenState, Running>()
		.kind(Phases::Update)
		.each([](flecs::entity e, TweenSequence) {
			auto childrenIt = ecs_children(e.world(), e.id());

			while (ecs_children_next(&childrenIt)) {
				for (int j = 0; j < childrenIt.count; ++j) {
					auto child = flecs::entity(childrenIt.world, childrenIt.entities[j]);

					if (child.has<TweenState, Waiting>()) {
						child.add<TweenState, Running>();
					}

					if (!child.has<TweenState, Completed>()) {
						return;
					}
				}

				e.add<TweenState, Completed>();
			}
		});

	world.system<TweenSimultaneus, TweenState>("process tween simultaneous")
		.term_at(1).second(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, TweenSimultaneus, TweenState state) {
			const auto entity = it.entity(i);

			auto notCompletedTweens = entity.world().query_builder()
				.with(flecs::ChildOf, entity)
				.without<TweenState, Completed>()
				.build();

			if (!notCompletedTweens.first().is_valid()) {
				entity.add<TweenState, Completed>();

				return;
			}

			notCompletedTweens.each([state = it.pair(1).second()](flecs::entity child) {
				child.add<TweenState>(state);
			});
		});

	world.system<TweenLoop, TweenState>("process tween loop")
		.term_at(1).second(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, TweenLoop, TweenState state) {
			const auto entity = it.entity(i);

			auto notRunningTween = entity.world().query_builder()
				.with(flecs::ChildOf, entity)
				.without<TweenState, Running>()
				.build()
				.first();

			if (notRunningTween.is_valid()) {
				entity.children([state = it.pair(1).second()](flecs::entity child) {
					child.add<TweenNextState>(state);
				});
			}
		});

	world.system<Time, TweenDuration, TweenTimer>("tween timer reset")
		.term_at(0).singleton()
		.with<TweenState, Waiting>()
		.kind(Phases::Update)
		.each([](flecs::entity e, Time& t, TweenDuration& duration, TweenTimer& timer) {
			timer.value = 0;
		});

	world.system<Time, TweenDuration, TweenTimer>("tween timer set")
		.term_at(0).singleton()
		.with<TweenState, Running>()
		.kind(Phases::Update)
		.each([](flecs::entity e, Time& t, TweenDuration& duration, TweenTimer& timer) {
			if (timer.value >= duration.value) {
				e.add<TweenState, Completed>();

				return;
			}

			timer.value += t.deltaTime;
			timer.value = glm::clamp(timer.value, 0.f, duration.value);
		});

	world.system<Transform, FromTransform, ToTransform, TweenFunction, TweenTimer, TweenDuration>("tween transform")
		.with<TweenState, Running>()
		.with<TweenTarget>().second("$target")
		.term_at(0).src("$target")
		.term_at(3).second(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, Transform& tr, FromTransform from, ToTransform to, TweenFunction func, TweenTimer& timer, TweenDuration& duration) {
			const auto funcId = it.pair(3).second();
			const auto factor = tweenFunctions.at(funcId)(timer.value / duration.value);

			tr.translation = glm::mix(from->translation, to->translation, factor);
		});

	world.system("tween cleanup")
		.with<TweenState, Completed>()
		.without<Tween>(flecs::Wildcard).parent()
		.kind(Phases::Update)
		.each([](flecs::entity e) {
			e.destruct();
		});
}
