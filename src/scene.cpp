#include "scene.h"

using namespace ps;
using namespace core;

std::unique_ptr<Scene> Scene::create() {
	return std::unique_ptr<Scene>(new Scene);
}

flecs::entity Scene::getRootEntity() {
	return m_root;
}
