#include "module.h"
#include "ecsModule/common.h"
#include "ecsModule/timeModule/module.h"
#include "ecsModule/transformModule/module.h"
#include "ecsModule/uiModule/module.h"

#include "gtx/easing.hpp"
#include "spdlog/spdlog.h"

using namespace ps;

TweenModule::TweenModule(flecs::world world) {
	world.module<TweenModule>();

	world.import<TransformModule>();
	world.import<TimeModule>();
	world.import<UiModule>();

	world.component<Tween>();
	world.component<Single>();
	world.component<Sequence>();
	world.component<Simultaneous>();
	world.component<Loop>();
	world.component<TweenFrom>();
	world.component<TweenTo>();
	world.component<TweenFunction>().add(flecs::Union);
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
	world.component<TweenTarget>().add(flecs::Exclusive);
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

	using FromBackgroundColor = flecs::pair<TweenFrom, BackgroundColor>;
	using ToBackgroundColor   = flecs::pair<TweenTo, BackgroundColor>;

	using TweenSequence    = flecs::pair<Tween, Sequence>;
	using TweenSimultaneus = flecs::pair<Tween, Simultaneous>;
	using TweenSingle      = flecs::pair<Tween, Single>;
	using TweenLoop        = flecs::pair<Tween, Loop>;

	world.observer<Tween>("init (tween, *)")
		.term_at(0).second(flecs::Wildcard)
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Tween){
			e.add(flecs::OrderedChildren);
			e.add<TweenState, Waiting>();
		});

	world.observer<Tween>("init (tween, seq)")
		.term_at(0).second<Sequence>()
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Tween& tween){
			tween.processChildren = [](flecs::entity e) {
				spdlog::info("[seq]: iterate children {}", e.name());
				auto childrenIt = ecs_children(e.world(), e.id());

				while (ecs_children_next(&childrenIt)) {
					for (int i = 0; i < childrenIt.count; ++i) {
						auto child = flecs::entity(childrenIt.world, childrenIt.entities[i]);

						if (child.has<TweenNextState, Waiting>()) {
							spdlog::info("[seq]: run child {}", child.name());
							child.add<TweenNextState, Running>();
						}

						if (child.has<TweenNextState, Completed>()) {
							spdlog::info("[seq]: complete child {}", child.name());
							child.add<TweenState, Completed>();
						} else {
							return;
						}
					}

					spdlog::info("[seq]: completed");
					e.add<TweenState, Completed>();
					e.add<TweenNextState, Completed>();
				}
			};
		});

	world.observer<Tween>("init (tween, sim)")
		.term_at(0).second<Simultaneous>()
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Tween& tween){
			tween.processChildren = [](flecs::entity e) {
				auto notCompletedTweens = e.world().query_builder()
					.with(flecs::ChildOf, e)
					.without<TweenNextState, Completed>()
					.build();

				if (!notCompletedTweens.first().is_valid()) {
					e.add<TweenState, Completed>();
					e.add<TweenNextState, Completed>();

					return;
				}

				notCompletedTweens.each([](flecs::entity child) {
					child.add<TweenNextState, Running>();
				});
			};
		});

	world.observer<Tween>("init (tween, loop)")
		.term_at(0).second<Loop>()
		.event(flecs::OnAdd)
		.each([](flecs::entity e, Tween& tween){
			tween.processChildren = [](flecs::entity e) {
				spdlog::info("[loop]: iterate children {}", e.name());
				e.children([](flecs::entity child) {
					if (!child.has<TweenNextState, Running>()) {
						child.add<TweenState, Waiting>();
						child.add<TweenNextState, Running>();
					}
				});
			};
		});

	world.system("execute root tween")
		.with<Tween>(flecs::Wildcard)
		.with<TweenState, Waiting>()
		.without<TweenNextState>(flecs::Wildcard)
		.without<Tween>(flecs::Wildcard).parent()
		.write<TweenState>(flecs::Wildcard)
		.write<TweenNextState>(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::entity e) {
			e.add<TweenState, Running>();
			e.add<TweenNextState, Running>();
		});

	world.system<Tween>("tween bot up")
		.self().cascade().desc()
		.term_at(0).second(flecs::Wildcard)
		.with<TweenState, Running>()
		.immediate()
		.kind(Phases::Update)
		.run([](flecs::iter& it) {
			while (it.next()) {
				auto tweens = it.field<Tween>(0);
				for (auto i : it) {
					auto e = it.entity(i);
					spdlog::info("[tween]: walk from bottom {}", e.name());
					it.world().defer_suspend();
					tweens[i].processChildren(e);
					it.world().defer_resume();
				}
			}
		});

	world.system("tween init/reset")
		.with<TweenNextState, Running>()
		.without<TweenState, Running>()
		.without<Tween, Single>()
		.write<TweenState>(flecs::Wildcard)
		.write<TweenNextState>(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::entity e) {
			e.children([](flecs::entity child) {
				child.add<TweenState, Waiting>();
				child.add<TweenNextState, Waiting>();
			});
			e.add<TweenState, Running>();
			spdlog::info("[tween]: init/reset {}", e.name());
			spdlog::info("[tween]: has (TweenState, Running) {}", e.has<TweenState, Running>());
		});


	world.system<Tween>("tween up bot")
		.self().up()
		.term_at(0).second(flecs::Wildcard)
		.with<TweenState, Running>()
		.write<TweenState>(flecs::Wildcard)
		.write<TweenNextState>(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::entity e, Tween& tween) {
			spdlog::info("[tween]: walk from top {}", e.name());
			tween.processChildren(e);
		});

	world.system<TweenTimer>("tween timer reset")
		.without<TweenState, Running>()
		.with<TweenNextState, Running>()
		.write<TweenState>(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::entity e, TweenTimer& timer) {
			timer.value = 0;

			spdlog::info("[timer]: run {}", e.name());
			e.add<TweenState, Running>();
		});

	world.system<Time, TweenDuration, TweenTimer>("tween timer set")
		.term_at(0).singleton()
		.with<TweenState, Running>()
		.without<TweenNextState, Completed>()
		.write<TweenNextState>(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::entity e, Time& t, TweenDuration& duration, TweenTimer& timer) {
			spdlog::info("[timer]: countdown {} with value {}", e.name(), timer.value);

			timer.value += e.world().delta_time();
			timer.value = std::min(timer.value, duration.value);

			if (timer.value >= duration.value) {
				spdlog::info("[timer]: finish {}", e.name());
				e.add<TweenNextState, Completed>();
			}
		});

	world.system<Transform>()
		.with<ToTransform>()
		.with<TweenFrom>().second("$target")
		.with<TweenState, Running>()
		.without<FromTransform>()
		.term_at(0).src("$target")
		.write<TweenFrom>(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::entity e, Transform& t) {
			spdlog::info("[tween_from]: reset {}", e.name());
			e.set<TweenFrom, Transform>(t);
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

			spdlog::info("[tween]: from/to with {} factor", factor);

			const auto translation = glm::mix(glm::vec2(from->translation), glm::vec2(to->translation), factor);
			tr.translation.x = translation.x;
			tr.translation.y = translation.y;
			tr.scale = glm::mix(from->scale, to->scale, factor);
			tr.rotation = glm::mix(from->rotation, to->rotation, factor);
		});

	world.system<BackgroundColor>()
		.with<ToBackgroundColor>()
		.with<TweenFrom>().second("$target")
		.with<TweenState, Running>()
		.without<FromBackgroundColor>()
		.term_at(0).src("$target")
		.kind(Phases::Update)
		.each([](flecs::entity e, BackgroundColor& t) {
			e.set<TweenFrom, BackgroundColor>(t);
		});

	world.system<BackgroundColor, FromBackgroundColor, ToBackgroundColor, TweenFunction, TweenTimer, TweenDuration>("tween color")
		.with<TweenState, Running>()
		.with<TweenTarget>().second("$target")
		.term_at(0).src("$target")
		.term_at(3).second(flecs::Wildcard)
		.kind(Phases::Update)
		.each([](flecs::iter& it, size_t i, BackgroundColor& tbg, FromBackgroundColor from, ToBackgroundColor to, TweenFunction func, TweenTimer& timer, TweenDuration& duration) {
			const auto funcId = it.pair(3).second();
			const auto factor = tweenFunctions.at(funcId)(timer.value / duration.value);

			const auto res = glm::mix((glm::u8vec4)*from, (glm::u8vec4)*to, factor);

			tbg.r = res.r;
			tbg.g = res.g;
			tbg.b = res.b;
			tbg.a = res.a;
		});

	world.system("tween cleanup")
		.with<TweenNextState, Completed>()
		.without<Tween>(flecs::Wildcard).parent()
		.kind(Phases::Update)
		.each([](flecs::entity e) {
			e.destruct();
		});
}
