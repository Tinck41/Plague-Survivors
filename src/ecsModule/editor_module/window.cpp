#include "window.h"

#include "docking.h"
#include "ecsModule/editor_module/module.h"
#include "ecsModule/editor_module/components.h"
#include "ecsModule/textModule/module.h"
#include "ecsModule/ui_module/module.h"
#include "ecsModule/assetModule/module.h"
#include "ecsModule/utils.h"
#include "spdlog/spdlog.h"

#include <format>

using namespace ps;

std::pair<flecs::entity, flecs::entity> ps::create_window(flecs::world &world, WindowConfig config) {
	const auto no_resize = (config.flags & static_cast<std::uint8_t>(WindowFlags::NoResize)) != 0;
	const auto no_collapse = (config.flags & static_cast<std::uint8_t>(WindowFlags::NoCollapse)) != 0;
	const auto no_titlebar = (config.flags & static_cast<std::uint8_t>(WindowFlags::NoTitlebar)) != 0;
	const auto no_close = (config.flags & static_cast<std::uint8_t>(WindowFlags::NoClose)) != 0;
	const auto no_docking = (config.flags & static_cast<std::uint8_t>(WindowFlags::NoDocking)) != 0;
	const auto no_move = (config.flags & static_cast<std::uint8_t>(WindowFlags::NoMove)) != 0;
	const auto name = config.name;

	auto collapse_button_entity = flecs::entity::null();

	const auto sizing_policy = [&] -> std::pair<Node::SizePolicy, Node::SizePolicy> {
		if (config.size) {
			if (no_resize) {
				return { Node::Fixed{ config.size.value().x }, Node::Fixed{ config.size.value().y } };
			}

			return { Node::Fit{ .min = config.size.value().x }, Node::Fit{ .min = config.size.value().y } };
		}
		else if (no_resize) {
			return { Node::Grow{}, Node::Grow{} };
		}

		return { Node::Fit{}, Node::Fit{} };
	}();

	auto window_entity = world.entity(name.data())
		.set<BackgroundColor>(Color::from_hex("#282828"))
		.set<BorderColor>(Color::from_hex("#504945"))
		.add<Button>()
		.set<Node>({
			.sizing_policy = sizing_policy,
			.padding = { 4.f, 4.f, 4.f, 4.f },
			.grow_direction = Node::GrowDirection::Vertical,
			.absolute = true,
			.border_radius = 4.f,
			.border_width = 1.f,
		});

	if (!no_titlebar) {
		auto titlebar_entity = world.entity(window_entity, std::format("{}_tabs", name).c_str())
			.set<Node>({
				.sizing_policy = { Node::Grow{}, Node::Fit{} },
			})
			.set<BackgroundColor>(Color::from_hex("#928374"))
			.add<WindowTitlebar>();

		if (!no_move) {
			titlebar_entity
				.add<Button>()
				.add<DragTarget>(window_entity);
		}

		if (!no_collapse) {
			collapse_button_entity = world.entity(titlebar_entity, std::format("{}_collapse_button", name).c_str())
				.set<Node>({
					.sizing_policy = { Node::Fixed{ 28 }, Node::Fixed{ 28 } },
				})
				.set<BackgroundColor>(Color::from_hex("#282828"))
				.add<WindowCollapseButton>();

			window_entity.add<CollapseButton>(collapse_button_entity);
		}

		auto window_name_holder_entity = world.entity(titlebar_entity, std::format("{}_tab_name_holder", name).c_str())
			.set<BackgroundColor>(TRANSPARENT)
			.set<Node>({
				.sizing_policy = { Node::Grow{}, Node::Grow{} },
			});

		auto window_name_entity = world.entity(window_name_holder_entity, std::format("{}_tab_name", name).c_str())
			.set<Text>(std::string(name))
			.set<TextFont>({
				.handle = world.get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
				.size = 24,
			})
			.set<TextColor>(WHITE)
			.add<EditorNode>();

		if (!no_close) {
			auto close_button_entity = world.entity(titlebar_entity, std::format("{}_close_button", name).c_str())
				.set<BackgroundColor>(RED)
				.set<Node>({
					.sizing_policy = { Node::Fixed{ 28.f }, Node::Fixed{ 28.f }},
				})
				.add<Button>()
				.add<EditorNode>()
				.add<CloseTarget>(window_entity);
		}
	}

	auto content_entity = world.entity(window_entity, std::format("{}_content", name).c_str())
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
			.overflow = { Node::Overflow::Clip, Node::Overflow::Clip },
		})
		.set<BackgroundColor>(TRANSPARENT)
		.add<WindowContent>();

	if (collapse_button_entity.is_valid()) {
		collapse_button_entity.add<CollapseTarget>(content_entity);
	}

	if (!no_resize) {
		auto resize_entity = world.entity(window_entity, std::format("{}_resize_button", name).c_str())
			.set<Node>({
				.sizing_policy = { Node::Fixed{ 10 }, Node::Fixed{ 10 } },
				.self_alignment = { 1.f, 1.f },
				.absolute = true,
			})
			.set<BackgroundColor>(Color::from_hex("#ebdbb2"))
			.add<ResizeTarget>(window_entity)
			.add<ResizeButtonNode>();

		if (collapse_button_entity.is_valid()) {
			collapse_button_entity.add<CollapseTarget>(resize_entity);
		}

		window_entity.add<ResizeButton>(resize_entity);
	}

	if (!no_docking) {
		window_entity.add<DockingEnabled>();
	}

	window_entity
		.add<WindowContent>(content_entity)
		.set<EditorWindow>({
			.content = content_entity,
			.flags = config.flags,
		});

	if (!no_move) {
		window_entity
			.add<DragTarget>(window_entity);
	}

	return { window_entity, content_entity };
}

flecs::entity create_titlebar(flecs::world& world, flecs::entity& window, WindowFlags flags) {
	const auto name = window.name().c_str();
	const auto no_move = (flags & static_cast<std::uint8_t>(WindowFlags::NoMove)) != 0;
	const auto no_collapse = (flags & static_cast<std::uint8_t>(WindowFlags::NoCollapse)) != 0;
	const auto no_close = (flags & static_cast<std::uint8_t>(WindowFlags::NoClose)) != 0;

	auto titlebar_entity = world.entity(std::format("{}_titlebar", name).c_str())
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Fit{} },
		})
		.set<BackgroundColor>(Color::from_hex("#928374"))
		.add<WindowTitlebar>();

	if (!no_move) {
		titlebar_entity
			.add<Button>()
			.add<DragTarget>(window);
	}

	if (!no_collapse) {
		auto collapse_button_entity = world.entity(titlebar_entity, std::format("{}_collapse_button", name).c_str())
			.set<Node>({
				.sizing_policy = { Node::Fixed{ 28 }, Node::Fixed{ 28 } },
			})
			.set<BackgroundColor>(Color::from_hex("#282828"))
			.add<WindowCollapseButton>();

		window.add<CollapseButton>(collapse_button_entity);
	}

	auto window_name_holder_entity = world.entity(titlebar_entity, std::format("{}_tab_name_holder", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
		});

	auto window_name_entity = world.entity(window_name_holder_entity, std::format("{}_tab_name", name).c_str())
		.set<Text>(std::string(name))
		.set<TextFont>({
			.handle = world.get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
			.size = 24,
		})
		.set<TextColor>(WHITE)
		.add<EditorNode>();

	if (!no_close) {
		auto close_button_entity = world.entity(titlebar_entity, std::format("{}_close_button", name).c_str())
			.set<BackgroundColor>(RED)
			.set<Node>({
				.sizing_policy = { Node::Fixed{ 28.f }, Node::Fixed{ 28.f }},
			})
			.add<Button>()
			.add<EditorNode>()
			.add<CloseTarget>(window);
	}

	return titlebar_entity;
}

flecs::entity create_tabsbar(flecs::world& world, flecs::entity& window, WindowFlags flags) {
	const auto name = window.name().c_str();

	auto tabsbar_entity = world.entity(std::format("{}_tabsbar", name).c_str())
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Fit{} },
		})
		.set<BackgroundColor>(Color::from_hex("#928374"))
		.add<WindowTitlebar>();

	auto tab_selector_entity = world.entity(tabsbar_entity, std::format("{}_tab_selector", name).c_str())
		.set<Node>({
			.sizing_policy = { Node::Fixed{ 28 }, Node::Fixed{ 28 } },
		})
		.set<BackgroundColor>(Color::from_hex("#282828"));

	auto window_name_holder_entity = world.entity(tabsbar_entity, std::format("{}_tab_name_holder", name).c_str())
		.set<BackgroundColor>(TRANSPARENT)
		.set<Node>({
			.sizing_policy = { Node::Grow{}, Node::Grow{} },
		});

	auto window_name_entity = world.entity(window_name_holder_entity, std::format("{}_tab_name", name).c_str())
		.set<Text>(std::string(name))
		.set<TextFont>({
			.handle = world.get_ref<AssetStorage>()->load_font("assets/FreeSans.ttf"),
			.size = 24,
		})
		.set<TextColor>(WHITE)
		.add<EditorNode>();

	return tabsbar_entity;
}
