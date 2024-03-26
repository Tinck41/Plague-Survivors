#include "physicsController.h"
#include "box2d/b2_world.h"

using namespace ps;
using namespace physics;

std::unique_ptr<PhysicsController> PhysicsController::create() {
	return std::unique_ptr<PhysicsController>(new PhysicsController);
}

b2World* PhysicsController::getWorld() {
	return m_world.get();
}
