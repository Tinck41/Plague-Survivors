#include "relationshipHolder.h"

#include "core/gameObject.h"

#include <algorithm>

using namespace ps::core;

RelationshipHolder::RelationshipHolder(RelationshipHolder&& other) noexcept 
	: m_parent(std::move(other.m_parent))
	, m_children(std::move(other.m_children))
{}

RelationshipHolder& RelationshipHolder::operator=(RelationshipHolder&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	m_parent = std::move(other.m_parent);
	m_children = std::move(other.m_children);

	return *this;
}

GameObject* RelationshipHolder::findChild(const std::string& name) {
	auto it = std::find_if(m_children.begin(), m_children.end(), [name](std::unique_ptr<GameObject>& child) {
		return child->getName() == name;
	});

	if (it != m_children.end()) {
		return it->get();
	}

	return nullptr;
}

std::unique_ptr<GameObject> RelationshipHolder::extractChild(GameObject& child) {
	auto it = std::find_if(m_children.begin(), m_children.end(), [name = child.getName()](std::unique_ptr<GameObject>& child) {
		return child->getName() == name;
	});

	if (it != m_children.end()) {
		return std::move(*m_children.erase(it));
	}

	return nullptr;
}

const std::vector<std::unique_ptr<GameObject>>& RelationshipHolder::getChildren() const {
	return m_children;
}

void RelationshipHolder::addChild(std::unique_ptr<GameObject> child) {
	m_children.emplace_back(std::move(child));
}

void RelationshipHolder::removeChild(GameObject& child) {
	auto it = std::find_if(m_children.begin(), m_children.end(), [name = child.getName()](std::unique_ptr<GameObject>& child) {
		return child->getName() == name;
	});

	if (it != m_children.end()) {
		m_children.erase(it)->reset();
	}
}

void RelationshipHolder::setParent(GameObject* parent) {
	m_parent = parent;
}
