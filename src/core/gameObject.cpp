#include "gameObject.h"

#include "componentBase.h"

using namespace ps::core;

GameObject::GameObject(const std::string& name) : m_name(name) {}

GameObject::GameObject() : GameObject("gameObject") {}

GameObject::GameObject(GameObject&& other)  noexcept
	: m_name(std::move(other.m_name)) 
{
	for (const auto& child : m_children) {
		child->setParent(this);
	}

	for (const auto& component : m_components) {
		component.second->setOwner(this);
	}
}

GameObject& GameObject::operator=(GameObject&& other)  noexcept {
	if (this == &other) {
		return *this;
	}

	m_name = std::move(other.m_name);

	for (const auto& child : m_children) {
		child->setParent(this);
	}

	for (const auto& component : m_components) {
		component.second->setOwner(this);
	}

	return *this;
}

GameObject::~GameObject() = default;

void GameObject::setName(const std::string& name) {
	m_name = name;
}

const std::string& GameObject::getName() {
	return m_name;
}
