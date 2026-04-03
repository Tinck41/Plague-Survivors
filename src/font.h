#pragma once

#include "SDL3_ttf/SDL_ttf.h"

#include <string>

namespace ps {
	class Font {
	public:
		Font(TTF_Font* font, std::string path);
		~Font();

		operator TTF_Font*() const {
			return m_resource;
		}

		float get_size() const;

		const std::string& get_path() const;

		TTF_Font* get_resource() const {
			return m_resource;
		}
	private:
		TTF_Font* m_resource;
		std::string m_path;
	};
}
