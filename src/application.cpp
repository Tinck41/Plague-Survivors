#include "application.h"

#include "SFML/Graphics.hpp"
#include "ecsModule/components/physicsComponent.h"
#include "ecsModule/components/rendererComponent.h"
#include "ecsModule/components/timeComponent.h"
#include "ecsModule/ecsController.h"

using namespace ps;
using namespace core;

std::unique_ptr<Application> Application::create() {
	return std::unique_ptr<Application>(new Application);
}

Application::Application() {
}

void Application::init() {
	ecsModule::EcsControllerInstance::init(ecsModule::EcsController::create());
	ecsModule::EcsControllerInstance::getInstance()->init();
}

void Application::run() {
	auto& world = ecsModule::EcsControllerInstance::getInstance()->getWorld(); 

	while(world.get<ecsModule::RendererComponent>()->renderer->isOpen()) {
		world.progress(world.get<ecsModule::TimeComponent>()->deltaTime);
	}
}

void Application::shutdown() {
	auto& world = ecsModule::EcsControllerInstance::getInstance()->getWorld(); 

	world.remove<ecsModule::RendererComponent>();
	world.remove<ecsModule::PhysicsComponent>();
	world.remove<ecsModule::TimeComponent>();

	ecsModule::EcsControllerInstance::shutdown();
}
