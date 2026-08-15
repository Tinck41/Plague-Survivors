#pragma once

#include "utils/sdl.h"

namespace se {
	struct Mesh {
		GpuBuffer index_buffer;
		GpuBuffer vertex_buffer;
	};
}
