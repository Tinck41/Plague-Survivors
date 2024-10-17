#pragma once

#include "core/componentBase.h"

#include <memory>
#include <optional>
#include <unordered_map>

namespace ps::core {
	class ComponentHolder {
	public:
		ComponentHolder() = default;
		ComponentHolder(const ComponentHolder& other) = delete;
		ComponentHolder(ComponentHolder&& other) noexcept;
		ComponentHolder& operator=(const ComponentHolder& other) = delete;
		ComponentHolder& operator=(ComponentHolder&& other) noexcept;
		virtual ~ComponentHolder() = default;

		template<typename T>
		T& addCompponent(std::unique_ptr<T> component) {
			const auto hash = getHash<T>();

			if (m_components.count(hash) > 0) {
				return m_components.at(hash);
			}

			m_components[hash] = std::move(component);

			return m_components.at(hash);
		}

		template<typename T>
		std::optional<T&> getComponent() {
			const auto hash = getHash<T>();

			if (m_components.count(hash) > 0) {
				return m_components.at(hash);
			}

			return std::nullopt;
		}

		template<typename T>
		void removeComponent() {
			const auto hash = getHash<T>();

			if (m_components.count(hash) > 0) {
				m_components.erase(hash);
			}
		}

		template<typename T>
		std::unique_ptr<T> extractComponent() {
			const auto hash = getHash<T>();

			if (m_components.count(hash) > 0) {
				auto component = m_components.extract(hash);
			}

			return nullptr;
		}

	private:
		template<typename T>
		size_t getHash() const {
			using Type = typename std::remove_pointer<T>::type;
			return typeid(Type).hash_code();
		}

	protected:
		std::unordered_map<size_t, std::unique_ptr<ComponentBase>> m_components;
	};
}
