#pragma once

#include "SDL3_ttf/SDL_ttf.h"

namespace ps {
	class Font {
	public:
		Font(TTF_Font* font);
		~Font();

		operator TTF_Font*() const {
			return m_resource;
		}

		float get_size() const;

		TTF_Font* get_resource() const {
			return m_resource;
		}
	private:
		TTF_Font* m_resource;
	};
}
