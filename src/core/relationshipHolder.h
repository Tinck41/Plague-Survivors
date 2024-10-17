#pragma once

#include <memory>
#include <vector>

namespace ps::core {
	class GameObject;

	class RelationshipHolder {
	public:
		RelationshipHolder() = default;
		RelationshipHolder(const RelationshipHolder& other) = delete;
		RelationshipHolder(RelationshipHolder&& other) noexcept;
		RelationshipHolder& operator=(const RelationshipHolder& other) = delete;
		RelationshipHolder& operator=(RelationshipHolder&& other) noexcept;
		virtual ~RelationshipHolder() = default;

		GameObject* findChild(const std::string& name);
		std::unique_ptr<GameObject> extractChild(GameObject& child);
		const std::vector<std::unique_ptr<GameObject>>& getChildren() const;

		void addChild(std::unique_ptr<GameObject> child);
		void removeChild(GameObject& child);
		void setParent(GameObject* parent);
	protected:
		std::vector<std::unique_ptr<GameObject>> m_children;
		GameObject* m_parent;
	};
}
