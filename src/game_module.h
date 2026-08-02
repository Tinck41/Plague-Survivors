#pragma once

#include "flecs.h"

namespace se {
	class GameModule {
	public:
		GameModule(flecs::world& world);
	private:
#ifdef _WIN32
		HMODULE lib_;
#else
		void* lib_;
#endif	
	};
}
