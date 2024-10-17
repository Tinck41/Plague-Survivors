#include "componentHolder.h"

using namespace ps::core;

ComponentHolder::ComponentHolder(ComponentHolder&& other) noexcept
	: m_components(std::move(other.m_components))
{}

ComponentHolder& ComponentHolder::operator=(ComponentHolder&& other) noexcept {
	if (this == &other) {
		return *this;
	}
	
	m_components = std::move(other.m_components);

	return *this;
}
