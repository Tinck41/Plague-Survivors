#pragma once

#include <memory>

namespace ps::utils {
	template<typename T>
	class InstanceBase {
	public:
		static void init(std::unique_ptr<T> instance) {
			m_instance = std::move(instance);
		};

		static T* getInstance() {
			return m_instance.get();
		};

		static void shutdown() {
			delete m_instance.release();
		};
	private:
		static inline std::unique_ptr<T> m_instance;
	};
}
