#pragma once

#include <filesystem>

namespace se {
	class File {
	public:
		File(std::filesystem::path path);
		~File();

		operator char*();

		const void* get_raw() const;

		size_t get_size() const;
	private:
		void* m_data;
		size_t m_size;
	};
}
