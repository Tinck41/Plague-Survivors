#include "game_module.h"
#include "spdlog/spdlog.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using InitFn = void(*)(ecs_world_t*);

using namespace se;

GameModule::GameModule(flecs::world& world) {
	world.module<GameModule>();
#ifdef _WIN32
	lib_ = LoadLibrary("game.dll");
	auto init = (InitFn)GetProcAddress(lib_, "init");
#else
	lib_ = dlopen("game.so", RTLD_NOW | RTLD_GLOBAL);
	if (!lib_) {
		spdlog::error("[GameModule]: dlopen failed: {}", dlerror());
		return;
	}

	auto init = (InitFn)dlsym(lib_, "init");
	if (!init) {
		spdlog::error("[GameModule]: dlsym failed: {}", dlerror());
		return;
	}
#endif

	init(world.c_ptr());
}
