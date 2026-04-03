#include "font.h"

using namespace ps;

Font::Font(TTF_Font* font, std::string path) : m_resource(font), m_path(std::move(path)) {}

Font::~Font() {
	TTF_CloseFont(m_resource);
}

const std::string& Font::get_path() const {
	return m_path;
}

float Font::get_size() const {
	return TTF_GetFontSize(m_resource);
}
