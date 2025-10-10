#include "default_modules.h"

#include "assetModule/module.h"
#include "transformModule/module.h"
#include "cameraModule/module.h"
#include "renderModule/module.h"
#include "textModule/module.h"
#include "spriteModule/module.h"
#include "meshModule/module.h"
#include "inputModule/module.h"

using namespace ps;

DefaultModules::DefaultModules(flecs::world& world) {
	//world.module<DeffaultModule>();

	world.import<flecs::stats>();

	world.import<AssetModule>();
	world.import<TransformModule>();
	world.import<CameraModule>();
	world.import<RenderModule>();
	world.import<TextModule>();
	world.import<SpriteModule>();
	world.import<MeshModule>();
	world.import<InputModule>();
}
