#pragma once

#include "core/componentHolder.h"
#include "core/relationshipHolder.h"

#include <string>

namespace ps::core {
	class GameObject
		: public ComponentHolder
		, public RelationshipHolder
	{
	public:
		GameObject(const std::string& name);
		GameObject();
		GameObject(const GameObject& other) = delete;
		GameObject(GameObject&& other) noexcept;
		GameObject& operator=(const GameObject& other) = delete;
		GameObject& operator=(GameObject&& other) noexcept;
		~GameObject() override;

		void setName(const std::string& name);

		const std::string& getName();
	private:
		std::string m_name;
	};
}
