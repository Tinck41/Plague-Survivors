#pragma once

#include "utils/sdl.h"

namespace ps {
	struct Mesh {
		GpuBuffer index_buffer;
		GpuBuffer vertex_buffer;
	};
}
