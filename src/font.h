#pragma once

#include "SDL3_ttf/SDL_ttf.h"

namespace ps {
	class Font {
	public:
		Font(TTF_Font* font);
		~Font();

		float get_size() const;
	private:
		TTF_Font* m_resource;
	};
}
