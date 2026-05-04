#pragma once

#include "flecs.h"

namespace ps {
	struct EditorRoot {};

	struct EditorModule {
		EditorModule(flecs::world& world);
	};
}
