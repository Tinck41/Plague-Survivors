#include "default_modules.h"

#include "assetModule/module.h"
#include "ecsModule/debug_module/module.h"
#include "transformModule/module.h"
#include "cameraModule/module.h"
#include "renderModule/module.h"
#include "textModule/module.h"
#include "spriteModule/module.h"
#include "meshModule/module.h"
#include "inputModule/module.h"
#include "ui_module/module.h"
#include "ui_render_module/module.h"

using namespace ps;

DefaultModules::DefaultModules(flecs::world& world) {
	world.module<DefaultModules>();

	world.import<flecs::stats>();

	world.import<AssetModule>();
	world.import<TransformModule>();
	world.import<CameraModule>();
	world.import<RenderModule>();
	world.import<SpriteModule>();
	world.import<TextModule>();
	world.import<MeshModule>();
	world.import<InputModule>();
	world.import<UiModule>();
	world.import<UiRenderModule>();
	world.import<DebugModule>();
}
