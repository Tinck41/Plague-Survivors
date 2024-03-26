#include "timeSystem.h"

#include "ecsModule/components/timeComponent.h"
#include "ecsModule/ecsController.h"
#include "SFML/System.hpp"

using namespace ps;
using namespace ecsModule;

void TimeSystem::create() {
	auto& world = EcsControllerInstance::getInstance()->getWorld();

	world.system<TimeComponent>()
		.term_at(1).singleton()
		.kind(flecs::OnStore)
		.each([](TimeComponent& t) {
			t.deltaTime = t.deltaClock.restart().asSeconds();
		});

	world.set<TimeComponent>({});
}
