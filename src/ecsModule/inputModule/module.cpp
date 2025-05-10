#include "module.h"

#include "ecsModule/common.h"
#include "ecsModule/appModule/module.h"
#include "ecsModule/renderModule/module.h"

using namespace ps;

int PsKeyToRaylibKey(Key key) {
	switch(key) {
		case Key::Space:      return KEY_SPACE;
		case Key::Apostrophe: return KEY_APOSTROPHE;
		case Key::Comma:      return KEY_COMMA;
		//case Key::Hyphen:   return KEY_HYPHEN;
		case Key::Period:     return KEY_PERIOD;
		case Key::Slash:      return KEY_SLASH;
		case Key::Num0:       return KEY_KP_0;
		case Key::Num1:       return KEY_KP_1;
		case Key::Num2:       return KEY_KP_2;
		case Key::Num3:       return KEY_KP_3;
		case Key::Num4:       return KEY_KP_4;
		case Key::Num5:       return KEY_KP_5;
		case Key::Num6:       return KEY_KP_6;
		case Key::Num7:       return KEY_KP_7;
		case Key::Num8:       return KEY_KP_8;
		case Key::Num9:       return KEY_KP_9;
		case Key::Semicolon:  return KEY_SEMICOLON;
		case Key::Equal:      return KEY_EQUAL;
		case Key::A:          return KEY_A;
		case Key::B:          return KEY_B;
		case Key::C:          return KEY_C;
		case Key::D:          return KEY_D;
		case Key::E:          return KEY_E;
		case Key::F:          return KEY_F;
		case Key::G:          return KEY_G;
		case Key::H:          return KEY_H;
		case Key::I:          return KEY_I;
		case Key::J:          return KEY_J;
		case Key::K:          return KEY_K;
		case Key::L:          return KEY_L;
		case Key::M:          return KEY_M;
		case Key::N:          return KEY_N;
		case Key::O:          return KEY_O;
		case Key::P:          return KEY_P;
		case Key::Q:          return KEY_Q;
		case Key::R:          return KEY_R;
		case Key::S:          return KEY_S;
		case Key::T:          return KEY_T;
		case Key::U:          return KEY_U;
		case Key::V:          return KEY_V;
		case Key::W:          return KEY_W;
		case Key::X:          return KEY_X;
		case Key::Y:          return KEY_Y;
		case Key::Z:          return KEY_Z;
		case Key::LBracket:   return KEY_LEFT_BRACKET;
		case Key::Backslash:  return KEY_BACKSLASH;
		case Key::RBracket:   return KEY_RIGHT_BRACKET;
		case Key::Grave:      return KEY_GRAVE;
		case Key::Escape:     return KEY_ESCAPE;
		case Key::Enter:      return KEY_ENTER;
		case Key::Tab:        return KEY_TAB;
		case Key::Backspace:  return KEY_BACKSPACE;
		case Key::Insert:     return KEY_INSERT;
		case Key::Delete:     return KEY_DELETE;
		case Key::Right:      return KEY_RIGHT;
		case Key::Left:       return KEY_LEFT;
		case Key::Down:       return KEY_DOWN;
		case Key::Up:         return KEY_UP;
		case Key::PageUp:     return KEY_PAGE_UP;
		case Key::PageDown:   return KEY_PAGE_DOWN;
		case Key::Home:       return KEY_HOME;
		case Key::End:        return KEY_END;
		case Key::LShift:     return KEY_LEFT_SHIFT;
		case Key::LControl:   return KEY_LEFT_CONTROL;
		case Key::LAlt:       return KEY_LEFT_ALT;
		case Key::RShift:     return KEY_RIGHT_SHIFT;
		case Key::RControl:   return KEY_RIGHT_CONTROL;
		case Key::RAlt:       return KEY_RIGHT_ALT;
		default:              return KEY_NULL;
	}
}

Key RaylibKeyToPsKey(int key) {
	switch(key) {
		case KEY_SPACE:         return Key::Space;
		case KEY_APOSTROPHE:    return Key::Apostrophe;
		case KEY_COMMA:         return Key::Comma;
		//case KEY_HYPHEN:      return Key::Hyphen;
		case KEY_PERIOD:        return Key::Period;
		case KEY_SLASH:         return Key::Slash;
		case KEY_KP_0:          return Key::Num0;
		case KEY_KP_1:          return Key::Num1;
		case KEY_KP_2:          return Key::Num2;
		case KEY_KP_3:          return Key::Num3;
		case KEY_KP_4:          return Key::Num4;
		case KEY_KP_5:          return Key::Num5;
		case KEY_KP_6:          return Key::Num6;
		case KEY_KP_7:          return Key::Num7;
		case KEY_KP_8:          return Key::Num8;
		case KEY_KP_9:          return Key::Num9;
		case KEY_SEMICOLON:     return Key::Semicolon;
		case KEY_EQUAL:         return Key::Equal;
		case KEY_A:             return Key::A;
		case KEY_B:             return Key::B;
		case KEY_C:             return Key::C;
		case KEY_D:             return Key::D;
		case KEY_E:             return Key::E;
		case KEY_F:             return Key::F;
		case KEY_G:             return Key::G;
		case KEY_H:             return Key::H;
		case KEY_I:             return Key::I;
		case KEY_J:             return Key::J;
		case KEY_K:             return Key::K;
		case KEY_L:             return Key::L;
		case KEY_M:             return Key::M;
		case KEY_N:             return Key::N;
		case KEY_O:             return Key::O;
		case KEY_P:             return Key::P;
		case KEY_Q:             return Key::Q;
		case KEY_R:             return Key::R;
		case KEY_S:             return Key::S;
		case KEY_T:             return Key::T;
		case KEY_U:             return Key::U;
		case KEY_V:             return Key::V;
		case KEY_W:             return Key::W;
		case KEY_X:             return Key::X;
		case KEY_Y:             return Key::Y;
		case KEY_Z:             return Key::Z;
		case KEY_LEFT_BRACKET:  return Key::LBracket;
		case KEY_BACKSLASH:     return Key::Backslash;
		case KEY_RIGHT_BRACKET: return Key::RBracket;
		case KEY_GRAVE:         return Key::Grave;
		case KEY_ESCAPE:        return Key::Escape;
		case KEY_ENTER:         return Key::Enter;
		case KEY_TAB:           return Key::Tab;
		case KEY_BACKSPACE:     return Key::Backspace;
		case KEY_INSERT:        return Key::Insert;
		case KEY_DELETE:        return Key::Delete;
		case KEY_RIGHT:         return Key::Right;
		case KEY_LEFT:          return Key::Left;
		case KEY_DOWN:          return Key::Down;
		case KEY_UP:            return Key::Up;
		case KEY_PAGE_UP:       return Key::PageUp;
		case KEY_PAGE_DOWN:     return Key::PageDown;
		case KEY_HOME:          return Key::Home;
		case KEY_END:           return Key::End;
		case KEY_LEFT_SHIFT:    return Key::LShift;
		case KEY_LEFT_CONTROL:  return Key::LControl;
		case KEY_LEFT_ALT:      return Key::LAlt;
		case KEY_RIGHT_SHIFT:   return Key::RShift;
		case KEY_RIGHT_CONTROL: return Key::RControl;
		case KEY_RIGHT_ALT:     return Key::RAlt;
		default:                return Key::Unknown;
	}
}

InputModule::InputModule(flecs::world& world) {
	world.module<InputModule>();

	world.import<RenderModule>();

	world.component<Input>();

	world.set<Input>({});

	world.system<Window>()
		.term_at(0).singleton()
		.kind(Phases::HandleInput)
		.each([](flecs::iter& it, size_t, Window& w) {
			if (IsWindowResized() && !IsWindowFullscreen()) {
				w.width = GetScreenWidth();
				w.height = GetScreenHeight();

				SetWindowSize(w.width, w.height);
			} else if ((w.width != GetScreenWidth() || w.height != GetScreenHeight()) && !IsWindowFullscreen()) {
				SetWindowSize(w.width, w.height);
			}
		});

	world.system<Input>()
		.kind(Phases::HandleInput)
		.term_at(0).singleton()
		.each([](Input& i) {
			for (auto& [key, state] : i.keys) {
				state.pressed = false;
				state.released = false;
				state.remain &= !IsKeyReleased(PsKeyToRaylibKey(key));
			}

			i.mouse.left.pressed = false;
			i.mouse.left.released = false;
			i.mouse.right.pressed = false;
			i.mouse.right.released = false;
			i.mouse.moved = false;

			while (const auto key = GetKeyPressed()) {
				const auto psKey = RaylibKeyToPsKey(key);

				if (!i.keys[psKey].remain) {
					i.keys[psKey].pressed = true;
					i.keys[psKey].remain = true;
				}
			}

			const auto newPos = GetMousePosition();
			if (newPos.x != i.mouse.position.x || newPos.y != i.mouse.position.y) {
				i.mouse.position.x = newPos.x;
				i.mouse.position.y = newPos.y;
				i.mouse.moved = true;
			}

			if (IsMouseButtonPressed(MouseButton::MOUSE_BUTTON_LEFT)) {
				if (!i.mouse.left.remain) {
					i.mouse.left.pressed = true;
					i.mouse.left.remain = true;
				}
			} else if (IsMouseButtonReleased(MouseButton::MOUSE_BUTTON_LEFT)) {
				i.mouse.left.pressed = false;
				i.mouse.left.remain = false;
				i.mouse.left.released = true;
			}

			if (IsMouseButtonPressed(MouseButton::MOUSE_BUTTON_RIGHT)) {
				if (!i.mouse.right.remain) {
					i.mouse.right.pressed = true;
					i.mouse.right.remain = true;
				}
			} else if (IsMouseButtonReleased(MouseButton::MOUSE_BUTTON_RIGHT)) {
				i.mouse.left.pressed = false;
				i.mouse.left.remain = false;
				i.mouse.left.released = true;
			}
		});
}
