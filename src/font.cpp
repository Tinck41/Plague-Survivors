#include "font.h"

using namespace ps;

Font::Font(TTF_Font* font) : m_resource(font) {}

Font::~Font() {
	TTF_CloseFont(m_resource);
}

float Font::get_size() const {
	return TTF_GetFontSize(m_resource);
}
