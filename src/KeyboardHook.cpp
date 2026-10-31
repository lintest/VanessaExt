#include "stdafx.h"
#include "KeyboardHook.h"

#ifdef _WINDOWS

static AddInNative* s_addin = nullptr;

static std::u16string s_message = u"HOTKEY";

static HHOOK hKeyboardHook = NULL;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION) {
		WORD vkCode = LOWORD(wParam);
		WORD keyFlags = HIWORD(lParam);
		if (s_addin && vkCode == 0x48
			&& (keyFlags & KF_ALTDOWN)
			&& !(keyFlags & KF_REPEAT)
			&& !(keyFlags & KF_UP))
		{
			s_addin->ExternalEvent(s_message.c_str(), u"ALT+H");
		}
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

static bool GetLibraryFile(std::wstring& path)
{
	path.resize(MAX_PATH);
	while (true) {
		DWORD res = GetModuleFileName(hModule, path.data(), (DWORD)path.size());
		if (res && res < path.size()) return true;
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return false;
		path.resize(path.size() * 2);
	}
}

static HMODULE LoadHookLibrary()
{
	std::wstring path;
	return GetLibraryFile(path) ? LoadLibrary(path.c_str()) : nullptr;
}

typedef void(__cdecl* SetKeyboardHookProc)(void* addin);

std::string KeyboardHook::Hook(AddInNative* addin, const std::u16string& msg)
{
	auto hLibrary = LoadHookLibrary();
	if (hLibrary) {
		auto proc = (SetKeyboardHookProc)GetProcAddress(hLibrary, "SetKeyboardHook");
		if (addin) s_message = msg.empty() ? u"HOTKEY" : msg;
		if (proc) { proc(addin); return {}; }
	}
	return "Keyboard hook error";
}

extern "C" {
	__declspec(dllexport) void __cdecl SetKeyboardHook(void* addin)
	{
		s_addin = (AddInNative*)addin;

		if (hKeyboardHook)
			UnhookWindowsHookEx(hKeyboardHook);

		hKeyboardHook = NULL;

		if (addin)
			hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, &KeyboardHookProc, hModule, NULL);
	}
}

#else //_WINDOWS

// 1C:Enterprise on Linux is an X11 client, including in Wayland sessions where
// it runs through XWayland. A single polling implementation therefore covers
// both cases: XWayland only forwards key events while the 1C window is active.
//
// Only dependencies already used by the component are involved:
// - X11 (linked for the Linux build in CMakeLists.txt);
// - the bundled json.hpp (included through stdafx.h);
// - the C++ standard library (std::thread, std::atomic).

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>
#include <unistd.h>

namespace {

	// A single hotkey description. `key` in the JSON payload is a Windows virtual
	// key code (as used by 1C's СочетаниеКлавиш), so the same scripts work on both
	// platforms.
	struct Hotkey {
		KeySym keysym = NoSymbol;
		KeyCode keycode = 0;
		bool ctrl = false;
		bool alt = false;
		bool shift = false;
		bool meta = false;
		std::u16string message = u"HOTKEY";
		std::u16string data;
	};

	// Minimal RAII wrapper over the X11 connection.
	struct XDisplay {
		Display* display = XOpenDisplay(nullptr);
		~XDisplay() { if (display) XCloseDisplay(display); }
		operator Display*() const { return display; }
		explicit operator bool() const { return display != nullptr; }
	};

	// Reads the first 32-bit property value. Used for _NET_WM_PID and
	// _NET_ACTIVE_WINDOW instead of the EWMH helpers from XWinBase.h.
	unsigned long GetCardinalProperty(Display* display, Window window, const char* name)
	{
		unsigned long result = 0;
		Atom type = None;
		int format = 0;
		unsigned long count = 0, bytes_after = 0;
		unsigned char* data = nullptr;
		if (XGetWindowProperty(display, window, XInternAtom(display, name, True),
				0, 1, False, AnyPropertyType, &type, &format, &count, &bytes_after, &data) == Success
			&& data && format == 32 && count > 0) {
			result = *(unsigned long*)data;
		}
		if (data) XFree(data);
		return result;
	}

	std::string LowerAscii(std::string value)
	{
		for (char& c : value) {
			if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
		}
		return value;
	}

	std::string GetString(const JSON& object, const char* name, const char* fallback = "")
	{
		auto it = object.find(name);
		return it != object.end() && it->is_string() ? it->get<std::string>() : fallback;
	}

	bool GetBool(const JSON& object, std::initializer_list<const char*> names)
	{
		for (const char* name : names) {
			auto it = object.find(name);
			if (it == object.end()) continue;
			if (it->is_boolean()) return it->get<bool>();
			if (it->is_number_integer()) return it->get<int64_t>() != 0;
		}
		return false;
	}

	JSON NormalizeObject(const JSON& value)
	{
		JSON result = JSON::object();
		if (!value.is_object()) return result;
		for (auto it = value.begin(); it != value.end(); ++it) {
			result[LowerAscii(it.key())] = it.value();
		}
		return result;
	}

	// Translates a Windows virtual key code to an X11 keysym.
	KeySym KeysymFromVirtualKey(int64_t key)
	{
		if (key >= 'A' && key <= 'Z') return XK_a + (key - 'A');
		if (key >= '0' && key <= '9') return XK_0 + (key - '0');
		if (key >= 0x70 && key <= 0x87) return XK_F1 + (key - 0x70);
		if (key >= 0x60 && key <= 0x69) return XK_KP_0 + (key - 0x60);

		switch (key) {
		case 0x08: return XK_BackSpace;
		case 0x09: return XK_Tab;
		case 0x0C: return XK_Clear;
		case 0x0D: return XK_Return;
		case 0x10: return XK_Shift_L;
		case 0x11: return XK_Control_L;
		case 0x12: return XK_Alt_L;
		case 0x13: return XK_Pause;
		case 0x14: return XK_Caps_Lock;
		case 0x1B: return XK_Escape;
		case 0x20: return XK_space;
		case 0x21: return XK_Page_Up;
		case 0x22: return XK_Page_Down;
		case 0x23: return XK_End;
		case 0x24: return XK_Home;
		case 0x25: return XK_Left;
		case 0x26: return XK_Up;
		case 0x27: return XK_Right;
		case 0x28: return XK_Down;
		case 0x2C: return XK_Print;
		case 0x2D: return XK_Insert;
		case 0x2E: return XK_Delete;
		case 0x5B: return XK_Super_L;
		case 0x5C: return XK_Super_R;
		case 0x5D: return XK_Menu;
		case 0x6A: return XK_KP_Multiply;
		case 0x6B: return XK_KP_Add;
		case 0x6D: return XK_KP_Subtract;
		case 0x6E: return XK_KP_Decimal;
		case 0x6F: return XK_KP_Divide;
		case 0x90: return XK_Num_Lock;
		case 0x91: return XK_Scroll_Lock;
		case 0xA0: return XK_Shift_L;
		case 0xA1: return XK_Shift_R;
		case 0xA2: return XK_Control_L;
		case 0xA3: return XK_Control_R;
		case 0xA4: return XK_Alt_L;
		case 0xA5: return XK_Alt_R;
		case 0xBA: return XK_semicolon;
		case 0xBB: return XK_equal;
		case 0xBC: return XK_comma;
		case 0xBD: return XK_minus;
		case 0xBE: return XK_period;
		case 0xBF: return XK_slash;
		case 0xC0: return XK_grave;
		case 0xDB: return XK_bracketleft;
		case 0xDC: return XK_backslash;
		case 0xDD: return XK_bracketright;
		case 0xDE: return XK_apostrophe;
		default: break;
		}

		// Allow callers to pass an X11 keysym directly.
		return (KeySym)key;
	}

	// The first keysym of a key code depends on the active layout, so for
	// non-Latin layouts look through all levels and groups.
	KeyCode FindKeycode(Display* display, KeySym keysym)
	{
		if (!display || keysym == NoSymbol) return 0;

		KeyCode keycode = XKeysymToKeycode(display, keysym);
		if (keycode) return keycode;

		int min = 0, max = 0, per_code = 0;
		XDisplayKeycodes(display, &min, &max);
		KeySym* map = XGetKeyboardMapping(display, (KeyCode)min, max - min + 1, &per_code);
		if (!map) return 0;

		for (int code = min; code <= max && !keycode; ++code) {
			for (int level = 0; level < per_code; ++level) {
				if (map[(code - min) * per_code + level] == keysym) {
					keycode = (KeyCode)code;
					break;
				}
			}
		}

		XFree(map);
		return keycode;
	}

	std::u16string HotkeyName(const Hotkey& hotkey)
	{
		std::string result;
		if (hotkey.ctrl) result += "CTRL+";
		if (hotkey.alt) result += "ALT+";
		if (hotkey.shift) result += "SHIFT+";
		if (hotkey.meta) result += "WIN+";

		const char* name = XKeysymToString(hotkey.keysym);
		std::string key = name ? name : "";
		for (char& c : key) {
			if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
		}
		result += key;
		return MB2WCHAR(result);
	}

	void AddHotkey(const JSON& raw, std::vector<Hotkey>& hotkeys)
	{
		if (!raw.is_object()) return;

		JSON object = NormalizeObject(raw);

		auto key = object.find("key");
		if (key == object.end()) return;

		KeySym keysym = NoSymbol;
		if (key->is_number()) {
			keysym = KeysymFromVirtualKey(key->get<int64_t>());
		}
		else if (key->is_string()) {
			std::string name = key->get<std::string>();
			if (!name.empty()) {
				keysym = XStringToKeysym(name.c_str());
				if (keysym == NoSymbol) {
					try {
						keysym = KeysymFromVirtualKey(std::stoll(name));
					}
					catch (...) {
						keysym = NoSymbol;
					}
				}
			}
		}
		if (keysym == NoSymbol) return;

		Hotkey hotkey;
		hotkey.keysym = keysym;
		hotkey.ctrl = GetBool(object, { "ctrl", "control" });
		hotkey.alt = GetBool(object, { "alt", "menu" });
		hotkey.shift = GetBool(object, { "shift" });
		hotkey.meta = GetBool(object, { "win", "meta", "super" });

		std::string message = GetString(object, "msg", "HOTKEY");
		hotkey.message = message.empty() ? u"HOTKEY" : MB2WCHAR(message);

		std::string data = GetString(object, "data");
		hotkey.data = data.empty() ? HotkeyName(hotkey) : MB2WCHAR(data);

		hotkeys.push_back(hotkey);
	}

	std::string ParseHotkeys(const std::string& text, std::vector<Hotkey>& hotkeys)
	{
		size_t pos = text.find_first_not_of(" \t\r\n");
		if (pos == std::string::npos || (text[pos] != '[' && text[pos] != '{')) {
			// Compatibility with the Windows implementation: a plain string is the
			// event name for the built-in ALT+H shortcut.
			Hotkey hotkey;
			hotkey.keysym = XK_h;
			hotkey.alt = true;
			hotkey.message = text.empty() ? u"HOTKEY" : MB2WCHAR(text);
			hotkey.data = u"ALT+H";
			hotkeys.push_back(hotkey);
			return {};
		}

		JSON root;
		try {
			root = JSON::parse(text);
		}
		catch (const std::exception& e) {
			return std::string("Keyboard hook error: ") + e.what();
		}

		if (root.is_array()) {
			for (const auto& item : root) AddHotkey(item, hotkeys);
		}
		else {
			JSON object = NormalizeObject(root);
			auto list = object.find("hotkeys");
			if (list != object.end() && list->is_array()) {
				for (const auto& item : *list) AddHotkey(item, hotkeys);
			}
			else {
				AddHotkey(root, hotkeys);
			}
		}

		if (hotkeys.empty()) return "Keyboard hook error: no hotkeys";
		return {};
	}

	bool IsKeyDown(const char* keys, KeyCode keycode)
	{
		return keycode && (keys[keycode >> 3] & (1 << (keycode & 7)));
	}

	// Walks up the window tree until a window of this process is found.
	bool WindowBelongsTo(Display* display, Window window, pid_t pid)
	{
		Window root = DefaultRootWindow(display);
		for (int depth = 0; window && window != root && depth < 64; ++depth) {
			if (GetCardinalProperty(display, window, "_NET_WM_PID") == (unsigned long)pid) return true;

			Window root_return = 0, parent = 0;
			Window* children = nullptr;
			unsigned int count = 0;
			if (!XQueryTree(display, window, &root_return, &parent, &children, &count)) break;
			if (children) XFree(children);
			if (!parent || parent == window) break;
			window = parent;
		}
		return false;
	}

	// True when the keyboard focus belongs to this process, i.e. to 1C.
	bool IsOwnProcessFocused(Display* display, pid_t pid)
	{
		Window focus = 0;
		int revert = 0;
		if (XGetInputFocus(display, &focus, &revert)
			&& focus != None && focus != PointerRoot) {
			return WindowBelongsTo(display, focus, pid);
		}

		Window active = (Window)GetCardinalProperty(display, DefaultRootWindow(display), "_NET_ACTIVE_WINDOW");
		return active && WindowBelongsTo(display, active, pid);
	}

	class LinuxKeyboardHook
	{
	private:
		AddInNative* m_addin = nullptr;
		std::vector<Hotkey> m_hotkeys;
		std::atomic<bool> m_stop{ false };
		std::thread m_thread;

		void Run()
		{
			// The worker thread uses its own X11 connection.
			XDisplay display;
			if (!display) return;

			enum { ModCtrl = 0, ModAlt, ModShift, ModMeta, ModCount };
			const KeyCode modifiers[ModCount][2] = {
				{ XKeysymToKeycode(display, XK_Control_L), XKeysymToKeycode(display, XK_Control_R) },
				{ XKeysymToKeycode(display, XK_Alt_L), XKeysymToKeycode(display, XK_Alt_R) },
				{ XKeysymToKeycode(display, XK_Shift_L), XKeysymToKeycode(display, XK_Shift_R) },
				{ XKeysymToKeycode(display, XK_Super_L), XKeysymToKeycode(display, XK_Super_R) },
			};

			const pid_t pid = getpid();
			char previous[32] = {};
			XQueryKeymap(display, previous);

			while (!m_stop.load(std::memory_order_relaxed)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(15));
				if (m_stop.load(std::memory_order_relaxed)) break;

				char current[32] = {};
				XQueryKeymap(display, current);

				bool pressed = false;
				for (size_t i = 0; i < sizeof(current) && !pressed; ++i) {
					if (current[i] & ~previous[i]) pressed = true;
				}

				if (pressed && IsOwnProcessFocused(display, pid)) {
					bool state[ModCount];
					for (int i = 0; i < ModCount; ++i) {
						state[i] = IsKeyDown(current, modifiers[i][0]) || IsKeyDown(current, modifiers[i][1]);
					}

					for (const auto& hotkey : m_hotkeys) {
						if (!IsKeyDown(current, hotkey.keycode) || IsKeyDown(previous, hotkey.keycode)) continue;
						if (hotkey.ctrl != state[ModCtrl] || hotkey.alt != state[ModAlt]
							|| hotkey.shift != state[ModShift] || hotkey.meta != state[ModMeta]) continue;

						if (m_addin) m_addin->ExternalEvent(hotkey.message, hotkey.data);
					}
				}

				std::memcpy(previous, current, sizeof(previous));
			}
		}

	public:
		~LinuxKeyboardHook()
		{
			Stop();
		}

		void Stop()
		{
			if (m_thread.joinable()) {
				m_stop.store(true, std::memory_order_relaxed);
				m_thread.join();
			}
			m_addin = nullptr;
			m_hotkeys.clear();
		}

		std::string Start(AddInNative* addin, const std::u16string& text)
		{
			Stop();

			std::string error = ParseHotkeys(WCHAR2MB((WCHAR_T*)text.c_str()), m_hotkeys);
			if (!error.empty()) {
				m_hotkeys.clear();
				return error;
			}

			// A temporary connection is enough to resolve the key codes; the worker
			// thread opens its own one.
			XDisplay display;
			if (!display) {
				m_hotkeys.clear();
				return "Keyboard hook error: cannot open X11 display";
			}

			for (auto it = m_hotkeys.begin(); it != m_hotkeys.end();) {
				it->keycode = FindKeycode(display, it->keysym);
				if (!it->keycode) it = m_hotkeys.erase(it);
				else ++it;
			}

			if (m_hotkeys.empty()) return "Keyboard hook error: unsupported keys";

			m_addin = addin;
			m_stop.store(false, std::memory_order_relaxed);

			try {
				m_thread = std::thread(&LinuxKeyboardHook::Run, this);
			}
			catch (const std::exception& e) {
				m_addin = nullptr;
				m_hotkeys.clear();
				return std::string("Keyboard hook error: ") + e.what();
			}
			return {};
		}
	};

	LinuxKeyboardHook s_hook;

} // namespace

std::string KeyboardHook::Hook(AddInNative* addin, const std::u16string& msg)
{
	if (!addin) {
		s_hook.Stop();
		return {};
	}
	return s_hook.Start(addin, msg);
}

#endif //_WINDOWS
