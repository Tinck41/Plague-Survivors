#pragma once

namespace ps::core {
	class GameObject;

	class ComponentBase {
	public:
		ComponentBase(GameObject* owner);
		virtual ~ComponentBase() = default;

		GameObject* getOwner();

		void setOwner(GameObject* owner);
	private:
		GameObject* m_owner;
	};
};
