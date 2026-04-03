#include "file.h"

#include "SDL3/SDL.h"

using namespace ps;

File::File(std::filesystem::path path) {
	m_data = SDL_LoadFile(path.string().c_str(), &m_size);
}

File::~File() {
	SDL_free(m_data);
}

File::operator char*() {
	return static_cast<char*>(m_data);
}

const void* File::get_raw() const {
	return m_data;
}

size_t File::get_size() const {
	return m_size;
}
