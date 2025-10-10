#include "core/application.h"
#include "ecsModule/default_modules.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/spriteModule/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/renderModule/module.h"
#include "ecsModule/common.h"
#include "spdlog/spdlog.h"

void init(flecs::world& world, ps::AssetStorage& storage, ps::RenderDevice& device) {
	world.entity("sprite")
		.set<ps::Sprite>({ .texture = storage.load_texture(*device.gpu, "assets/COUPON.png") });
	world.entity("text")
		.set<ps::Text>({ .string = "\nHello\nSDL3_ttf!", .font = storage.load_font("assets/FreeSans.ttf", 46) });
}

int main() {
	ps::Application::create()
		.add_module<ps::DefaultModules>()
		.add_system(ps::Phases::OnStart, &init)
		.run();
}

