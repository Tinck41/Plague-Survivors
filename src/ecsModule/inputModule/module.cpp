#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/appModule/module.h"

using namespace ps;

//Key SfmlKeyToPsKey(KeyboardKey key) {
//	switch(key) {
//		case KEY_SPACE:      return Key::Space;
//		case KEY_APOSTROPHE: return Key::Apostrophe;
//		case KEY_COMMA:      return Key::Comma;
//		//case KEY_HYPHEN:     return Key::Hyphen;
//		case KEY_PERIOD:     return Key::Period;
//		case KEY_SLASH:      return Key::Slash;
//		case KEY_KP_0:       return Key::Num0;
//		case KEY_KP_1:       return Key::Num1;
//		case KEY_KP_2:       return Key::Num2;
//		case KEY_KP_3:       return Key::Num3;
//		case KEY_KP_4:       return Key::Num4;
//		case KEY_KP_5:       return Key::Num5;
//		case KEY_KP_6:       return Key::Num6;
//		case KEY_KP_7:       return Key::Num7;
//		case KEY_KP_8:       return Key::Num8;
//		case KEY_KP_9:       return Key::Num9;
//		case KEY_SEMICOLON:  return Key::Semicolon;
//		case KEY_EQUAL:      return Key::Equal;
//		case KEY_A:          return Key::A;
//		case KEY_B:          return Key::B;
//		case KEY_C:          return Key::C;
//		case KEY_D:          return Key::D;
//		case KEY_E:          return Key::E;
//		case KEY_F:          return Key::F;
//		case KEY_G:          return Key::G;
//		case KEY_H:          return Key::H;
//		case KEY_I:          return Key::I;
//		case KEY_J:          return Key::J;
//		case KEY_K:          return Key::K;
//		case KEY_L:          return Key::L;
//		case KEY_M:          return Key::M;
//		case KEY_N:          return Key::N;
//		case KEY_O:          return Key::O;
//		case KEY_P:          return Key::P;
//		case KEY_Q:          return Key::Q;
//		case KEY_R:          return Key::R;
//		case KEY_S:          return Key::S;
//		case KEY_T:          return Key::T;
//		case KEY_U:          return Key::U;
//		case KEY_V:          return Key::V;
//		case KEY_W:          return Key::W;
//		case KEY_X:          return Key::X;
//		case KEY_Y:          return Key::Y;
//		case KEY_Z:          return Key::Z;
//		case KEY_LEFT_BRACKET:   return Key::LBracket;
//		case KEY_BACKSLASH:  return Key::Backslash;
//		case KEY_RIGHT_BRACKET:   return Key::RBracket;
//		case KEY_GRAVE:      return Key::Grave;
//		case KEY_ESCAPE:     return Key::Escape;
//		case KEY_ENTER:      return Key::Enter;
//		case KEY_TAB:        return Key::Tab;
//		case KEY_BACKSPACE:  return Key::Backspace;
//		case KEY_INSERT:     return Key::Insert;
//		case KEY_DELETE:     return Key::Delete;
//		case KEY_RIGHT:      return Key::Right;
//		case KEY_LEFT:       return Key::Left;
//		case KEY_DOWN:       return Key::Down;
//		case KEY_UP:         return Key::Up;
//		case KEY_PAGE_UP:     return Key::PageUp;
//		case KEY_PAGE_DOWN:   return Key::PageDown;
//		case KEY_HOME:       return Key::Home;
//		case KEY_END:        return Key::End;
//		case KEY_LEFT_SHIFT:     return Key::LShift;
//		case KEY_LEFT_CONTROL:   return Key::LControl;
//		case KEY_LEFT_ALT:       return Key::LAlt;
//		case KEY_RIGHT_SHIFT:     return Key::RShift;
//		case KEY_RIGHT_CONTROL:   return Key::RControl;
//		case KEY_RIGHT_ALT:       return Key::RAlt;
//		default:                            return Key::Unknown;
//	}
//}
//
//InputModule::InputModule(flecs::world& world) {
//	world.module<InputModule>();
//
//	world.component<Input>();
//
//	world.set<Input>({});
//
//	world.system<Application, Input>()
//		.kind(Phases::HandleInput)
//		.term_at(0).singleton()
//		.term_at(1).singleton()
//		.each([](Application& app, Input& i) {
//			for (auto& [_, state] : i.keys) {
//				state.pressed = false;
//				state.released = false;
//			}
//			i.mouse.left.pressed = false;
//			i.mouse.left.released = false;
//			i.mouse.right.pressed = false;
//			i.mouse.right.released = false;
//			i.mouse.moved = false;
//
//			while (const auto event = app.window.pollEvent()) {
//				if (event->is<sf::Event::Closed>()) {
//					app.window.close();
//				} else if (const auto resized = event->getIf<sf::Event::Resized>()) {
//					auto view = app.window.getView();
//					view.setSize({ static_cast<float>(resized->size.x), static_cast<float>(resized->size.y) });
//					app.window.setView(view);
//				} else if (const auto pressed = event->getIf<sf::Event::KeyPressed>()) {
//					auto key = SfmlKeyToPsKey(pressed->code);
//					if (!i.keys[key].remain) {
//						i.keys[key].pressed = true;
//						i.keys[key].remain = true;
//					}
//				} else if(const auto released = event->getIf<sf::Event::KeyReleased>()) {
//					auto key = SfmlKeyToPsKey(released->code);
//					i.keys[key].pressed = false;
//					i.keys[key].remain = false;
//					i.keys[key].released = true;
//				} else if (const auto moved = event->getIf<sf::Event::MouseMoved>()) {
//					const auto center = app.window.getView().getCenter();
//					const auto halfSize = app.window.getView().getSize() / 2.f;
//					const auto offset = center - halfSize;
//					const auto newPos = sf::Vector2f{ static_cast<float>(moved->position.x), static_cast<float>(moved->position.y) } + offset;
//					if (newPos != i.mouse.position) {
//						i.mouse.position = newPos;
//						i.mouse.moved = true;
//					}
//				} else if (const auto pressed = event->getIf<sf::Event::MouseButtonPressed>()) {
//					if (pressed->button == sf::Mouse::Button::Left) {
//						if (!i.mouse.left.remain) {
//							i.mouse.left.pressed = true;
//							i.mouse.left.remain = true;
//						}
//					} else if (pressed->button == sf::Mouse::Button::Right) {
//						if (!i.mouse.right.remain) {
//							i.mouse.right.pressed = true;
//							i.mouse.right.remain = true;
//						}
//					}
//				} else if (const auto released = event->getIf<sf::Event::MouseButtonReleased>()) {
//					if (released->button == sf::Mouse::Button::Left) {
//						i.mouse.left.pressed = false;
//						i.mouse.left.remain = false;
//						i.mouse.left.released = true;
//					} else if (released->button == sf::Mouse::Button::Right) {
//						i.mouse.right.pressed = false;
//						i.mouse.right.remain = false;
//						i.mouse.right.released = true;
//					}
//				}
//			};
//		});
//}
