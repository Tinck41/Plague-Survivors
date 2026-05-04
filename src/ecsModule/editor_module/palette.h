#pragma once

#include "color.h"

namespace ps {
	struct Palette {
		Color bg;
		Color text;
		Color highlight;
		Color select;
	};

	constexpr auto dark = Palette{
		.bg        = { 40, 40, 40 },
		.text      = { 0, 0, 0 },
		.highlight = { 146, 131, 116 },
		.select    = { 254, 128, 25 },
	};
}
