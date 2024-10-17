#include "application.h"

int main() {
	auto app = std::make_unique<ps::core::Application>();

	app->init();
	app->run();
	app->shutdown();

	return 0;
}
