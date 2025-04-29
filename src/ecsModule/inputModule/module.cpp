#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/appModule/module.h"

using namespace ps;

Key SfmlKeyToPsKey(sf::Keyboard::Key key) {
	switch(key) {
		case sf::Keyboard::Key::Space:      return Key::Space;
		case sf::Keyboard::Key::Apostrophe: return Key::Apostrophe;
		case sf::Keyboard::Key::Comma:      return Key::Comma;
		case sf::Keyboard::Key::Hyphen:     return Key::Hyphen;
		case sf::Keyboard::Key::Period:     return Key::Period;
		case sf::Keyboard::Key::Slash:      return Key::Slash;
		case sf::Keyboard::Key::Num0:       return Key::Num0;
		case sf::Keyboard::Key::Num1:       return Key::Num1;
		case sf::Keyboard::Key::Num2:       return Key::Num2;
		case sf::Keyboard::Key::Num3:       return Key::Num3;
		case sf::Keyboard::Key::Num4:       return Key::Num4;
		case sf::Keyboard::Key::Num5:       return Key::Num5;
		case sf::Keyboard::Key::Num6:       return Key::Num6;
		case sf::Keyboard::Key::Num7:       return Key::Num7;
		case sf::Keyboard::Key::Num8:       return Key::Num8;
		case sf::Keyboard::Key::Num9:       return Key::Num9;
		case sf::Keyboard::Key::Semicolon:  return Key::Semicolon;
		case sf::Keyboard::Key::Equal:      return Key::Equal;
		case sf::Keyboard::Key::A:          return Key::A;
		case sf::Keyboard::Key::B:          return Key::B;
		case sf::Keyboard::Key::C:          return Key::C;
		case sf::Keyboard::Key::D:          return Key::D;
		case sf::Keyboard::Key::E:          return Key::E;
		case sf::Keyboard::Key::F:          return Key::F;
		case sf::Keyboard::Key::G:          return Key::G;
		case sf::Keyboard::Key::H:          return Key::H;
		case sf::Keyboard::Key::I:          return Key::I;
		case sf::Keyboard::Key::J:          return Key::J;
		case sf::Keyboard::Key::K:          return Key::K;
		case sf::Keyboard::Key::L:          return Key::L;
		case sf::Keyboard::Key::M:          return Key::M;
		case sf::Keyboard::Key::N:          return Key::N;
		case sf::Keyboard::Key::O:          return Key::O;
		case sf::Keyboard::Key::P:          return Key::P;
		case sf::Keyboard::Key::Q:          return Key::Q;
		case sf::Keyboard::Key::R:          return Key::R;
		case sf::Keyboard::Key::S:          return Key::S;
		case sf::Keyboard::Key::T:          return Key::T;
		case sf::Keyboard::Key::U:          return Key::U;
		case sf::Keyboard::Key::V:          return Key::V;
		case sf::Keyboard::Key::W:          return Key::W;
		case sf::Keyboard::Key::X:          return Key::X;
		case sf::Keyboard::Key::Y:          return Key::Y;
		case sf::Keyboard::Key::Z:          return Key::Z;
		case sf::Keyboard::Key::LBracket:   return Key::LBracket;
		case sf::Keyboard::Key::Backslash:  return Key::Backslash;
		case sf::Keyboard::Key::RBracket:   return Key::RBracket;
		case sf::Keyboard::Key::Grave:      return Key::Grave;
		case sf::Keyboard::Key::Escape:     return Key::Escape;
		case sf::Keyboard::Key::Enter:      return Key::Enter;
		case sf::Keyboard::Key::Tab:        return Key::Tab;
		case sf::Keyboard::Key::Backspace:  return Key::Backspace;
		case sf::Keyboard::Key::Insert:     return Key::Insert;
		case sf::Keyboard::Key::Delete:     return Key::Delete;
		case sf::Keyboard::Key::Right:      return Key::Right;
		case sf::Keyboard::Key::Left:       return Key::Left;
		case sf::Keyboard::Key::Down:       return Key::Down;
		case sf::Keyboard::Key::Up:         return Key::Up;
		case sf::Keyboard::Key::PageUp:     return Key::PageUp;
		case sf::Keyboard::Key::PageDown:   return Key::PageDown;
		case sf::Keyboard::Key::Home:       return Key::Home;
		case sf::Keyboard::Key::End:        return Key::End;
		case sf::Keyboard::Key::LShift:     return Key::LShift;
		case sf::Keyboard::Key::LControl:   return Key::LControl;
		case sf::Keyboard::Key::LAlt:       return Key::LAlt;
		case sf::Keyboard::Key::RShift:     return Key::RShift;
		case sf::Keyboard::Key::RControl:   return Key::RControl;
		case sf::Keyboard::Key::RAlt:       return Key::RAlt;
		default:                            return Key::Unknown;
	}
}

InputModule::InputModule(flecs::world& world) {
	world.module<InputModule>();

	world.component<Input>();

	world.set<Input>({});

	world.system<Application, Input>()
		.kind(Phases::HandleInput)
		.term_at(0).singleton()
		.term_at(1).singleton()
		.each([](Application& app, Input& i) {
			for (auto& [_, state] : i.keys) {
				state.pressed = false;
				state.released = false;
			}
			i.mouse.left.pressed = false;
			i.mouse.left.released = false;
			i.mouse.right.pressed = false;
			i.mouse.right.released = false;
			i.mouse.moved = false;

			while (const auto event = app.window.pollEvent()) {
				if (event->is<sf::Event::Closed>()) {
					app.window.close();
				} else if (const auto resized = event->getIf<sf::Event::Resized>()) {
					auto view = app.window.getView();
					view.setSize({ static_cast<float>(resized->size.x), static_cast<float>(resized->size.y) });
					app.window.setView(view);
				} else if (const auto pressed = event->getIf<sf::Event::KeyPressed>()) {
					auto key = SfmlKeyToPsKey(pressed->code);
					if (!i.keys[key].remain) {
						i.keys[key].pressed = true;
						i.keys[key].remain = true;
					}
				} else if(const auto released = event->getIf<sf::Event::KeyReleased>()) {
					auto key = SfmlKeyToPsKey(released->code);
					i.keys[key].pressed = false;
					i.keys[key].remain = false;
					i.keys[key].released = true;
				} else if (const auto moved = event->getIf<sf::Event::MouseMoved>()) {
					const auto center = app.window.getView().getCenter();
					const auto halfSize = app.window.getView().getSize() / 2.f;
					const auto offset = center - halfSize;
					const auto newPos = sf::Vector2f{ static_cast<float>(moved->position.x), static_cast<float>(moved->position.y) } + offset;
					if (newPos != i.mouse.position) {
						i.mouse.position = newPos;
						i.mouse.moved = true;
					}
				} else if (const auto pressed = event->getIf<sf::Event::MouseButtonPressed>()) {
					if (pressed->button == sf::Mouse::Button::Left) {
						if (!i.mouse.left.remain) {
							i.mouse.left.pressed = true;
							i.mouse.left.remain = true;
						}
					} else if (pressed->button == sf::Mouse::Button::Right) {
						if (!i.mouse.right.remain) {
							i.mouse.right.pressed = true;
							i.mouse.right.remain = true;
						}
					}
				} else if (const auto released = event->getIf<sf::Event::MouseButtonReleased>()) {
					if (released->button == sf::Mouse::Button::Left) {
						i.mouse.left.pressed = false;
						i.mouse.left.remain = false;
						i.mouse.left.released = true;
					} else if (released->button == sf::Mouse::Button::Right) {
						i.mouse.right.pressed = false;
						i.mouse.right.remain = false;
						i.mouse.right.released = true;
					}
				}
			};
		});
}
