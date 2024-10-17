#include "componentBase.h"

#include "core/gameObject.h"

using namespace ps::core;

ComponentBase::ComponentBase(GameObject* owner) : m_owner(owner) {}

GameObject* ComponentBase::getOwner() {
	return m_owner;
}

void ComponentBase::setOwner(GameObject* owner) {
	// TODO: remove component from previous owner
	m_owner = owner;
}
