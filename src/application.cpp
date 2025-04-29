#include "application.h"

#include "SFML/Graphics.hpp"
#include "ecsModule/ecsController.h"
#include "ecsModule/timeModule/module.h"

using namespace ps;
using namespace core;

std::unique_ptr<Application> Application::create() {
	return std::unique_ptr<Application>(new Application);
}

void Application::init() {
	ecsModule::EcsControllerInstance::init(ecsModule::EcsController::create());
	ecsModule::EcsControllerInstance::getInstance()->init();
}

void Application::run() {
	auto& world = ecsModule::EcsControllerInstance::getInstance()->getWorld(); 

	//while(world.get<Renderer>()->renderer->isOpen()) {
	//	world.progress(world.get<Time>()->deltaTime);
	//}
}

void Application::shutdown() {
	ecsModule::EcsControllerInstance::shutdown();
}
