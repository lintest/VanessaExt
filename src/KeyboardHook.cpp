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
// it runs through XWayland. A single implementation therefore covers both
// cases: XWayland only forwards key events while the 1C window is active.
//
// The preferred mode uses the XRecord extension: the worker thread receives
// real key press/release events, so no fast keystrokes are missed and no
// polling is done. When XRecord is unavailable (some XWayland builds), the
// implementation falls back to polling the key map, but only while the 1C
// window is focused; between focus changes the worker sleeps in select() on
// the X connection and wakes up on root window property changes.
//
// The focus state is cached and refreshed event-driven: a tracker connection
// subscribes to _NET_ACTIVE_WINDOW changes on the root window, so the
// expensive process-tree check runs only on focus switches (or at most once
// per second as a fallback), never on every keystroke.
//
// Only dependencies already used by the component are involved:
// - X11 and libXtst (XRecord is part of Xtst), both linked in CMakeLists.txt;
// - the bundled json.hpp (included through stdafx.h);
// - the C++ standard library (std::thread, std::atomic, std::mutex).

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>

#include <sys/select.h>
#include <sys/time.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace {

	enum { ModCtrl = 0, ModAlt, ModShift, ModMeta, ModCount };

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

	// Modifier key codes change together with the keyboard mapping, so they
	// are re-resolved on MappingNotify and on periodic rebinding.
	void ResolveModifiers(Display* display, KeyCode (&modifiers)[ModCount][2])
	{
		const KeySym keysyms[ModCount][2] = {
			{ XK_Control_L, XK_Control_R },
			{ XK_Alt_L, XK_Alt_R },
			{ XK_Shift_L, XK_Shift_R },
			{ XK_Super_L, XK_Super_R },
		};
		for (int i = 0; i < ModCount; ++i) {
			for (int j = 0; j < 2; ++j) {
				modifiers[i][j] = XKeysymToKeycode(display, keysyms[i][j]);
			}
		}
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

	// Event-driven focus cache. Owns an X11 connection subscribed to root
	// window property changes (_NET_ACTIVE_WINDOW), so the cached focus state
	// is refreshed on focus switches without per-keystroke round trips. A
	// periodic revalidation covers servers that do not maintain the EWMH
	// property. The connection also receives MappingNotify, which the owner
	// consumes to re-resolve key codes.
	class FocusTracker {
		using clock = std::chrono::steady_clock;
	public:
		FocusTracker() : m_display(XOpenDisplay(nullptr))
		{
			if (m_display) {
				m_root = DefaultRootWindow(m_display);
				m_activeWindow = XInternAtom(m_display, "_NET_ACTIVE_WINDOW", True);
				XSelectInput(m_display, m_root, PropertyChangeMask);
			}
		}
		~FocusTracker() { if (m_display) XCloseDisplay(m_display); }
		FocusTracker(const FocusTracker&) = delete;
		FocusTracker& operator=(const FocusTracker&) = delete;

		operator Display*() const { return m_display; }
		explicit operator bool() const { return m_display != nullptr; }

		// Full (expensive) refresh of the cached focus state.
		bool Update(pid_t pid)
		{
			m_focused = m_display && IsOwnProcessFocused(m_display, pid);
			m_checked = clock::now();
			return m_focused;
		}

		// Cached state with a local event pump: pending events are processed
		// without round trips, the expensive check runs only after a reported
		// focus switch or when the fallback interval has elapsed.
		bool Focused(pid_t pid)
		{
			if (!m_display) return false;
			Drain();
			if (clock::now() - m_checked >= std::chrono::seconds(1)) Update(pid);
			return m_focused;
		}

		// Returns and resets the pending keyboard mapping change flag.
		bool ConsumeMappingChange()
		{
			if (m_display) Drain();
			bool changed = m_mappingChanged;
			m_mappingChanged = false;
			return changed;
		}

	private:
		void Drain()
		{
			while (XPending(m_display)) {
				XEvent event;
				XNextEvent(m_display, &event);
				if (event.type == PropertyNotify
					&& event.xproperty.window == m_root
					&& event.xproperty.atom == m_activeWindow) {
					// Active window changed: force revalidation on next query.
					m_checked = clock::time_point();
				}
				else if (event.type == MappingNotify) {
					XRefreshKeyboardMapping(&event.xmapping);
					m_mappingChanged = true;
				}
			}
		}

		Display* m_display = nullptr;
		Window m_root = 0;
		Atom m_activeWindow = None;
		bool m_focused = false;
		bool m_mappingChanged = false;
		clock::time_point m_checked = clock::time_point();
	};

	// XRecord callback. Runs on the worker thread between replies of the data
	// connection; the query connection may be used here, the data one may not.
	void RecordCallback(XPointer closure, XRecordInterceptData* data);

	class LinuxKeyboardHook;

	// Per-run state of the XRecord worker. Owned by the worker thread, passed
	// to the record callback as an opaque pointer.
	struct RecordState {
		LinuxKeyboardHook* hook = nullptr;
		Display* query = nullptr;
		FocusTracker* focus = nullptr;
		pid_t pid = 0;
		KeyCode modifiers[ModCount][2] = {};
		bool down[256] = {};
		Time lastRelease[256] = {};
		Time lastRebind = 0;
	};

	class LinuxKeyboardHook
	{
	private:
		AddInNative* m_addin = nullptr;
		std::vector<Hotkey> m_hotkeys;
		std::atomic<bool> m_stop{ false };
		std::thread m_thread;

		// XRecord lifecycle objects. Created by Start() on the calling thread,
		// destroyed by Stop() after the worker has been joined; the worker
		// itself never frees them.
		Display* m_recordCtrl = nullptr;
		XRecordContext m_recordContext = 0;
		std::mutex m_recordMutex;

		// Re-resolves hotkey key codes. Called when the hook starts and again
		// when the keyboard mapping may have changed (MappingNotify in polling
		// mode, periodically in record mode).
		void ResolveKeycodes(Display* display)
		{
			if (!display) return;
			for (auto& hotkey : m_hotkeys) {
				KeyCode code = FindKeycode(display, hotkey.keysym);
				if (code) hotkey.keycode = code;
			}
		}

		// Record mode: blocks in XRecordEnableContext until Stop() disables the
		// context. Returns false when the record stream ended prematurely, so
		// the caller can switch to the polling fallback.
		bool RunRecord(Display* ctrl, XRecordContext context)
		{
			// The data connection must differ from the control one, because
			// XRecordEnableContext blocks this connection until the context is
			// disabled. The focus tracker owns the query connection used for
			// Xlib round trips made from the callback (focus checks, rebinding).
			XDisplay data;
			FocusTracker focus;
			if (!data || !focus) return false;

			RecordState state;
			state.hook = this;
			state.focus = &focus;
			state.query = focus;
			state.pid = getpid();
			ResolveModifiers(state.query, state.modifiers);
			state.focus->Update(state.pid);

			// Seed the key-down map so a combination held while the hook was
			// started is still recognized.
			char keys[32] = {};
			XQueryKeymap(state.query, keys);
			for (int code = 0; code < 256; ++code) {
				state.down[code] = IsKeyDown(keys, (KeyCode)code);
			}

			Display* dataX11 = data;
			XRecordEnableContext(dataX11, context, RecordCallback, (XPointer)&state);

			return m_stop.load(std::memory_order_acquire);
		}

		// Polling fallback: unlike the old implementation the worker sleeps in
		// select() on the X connection and polls the key map only while the 1C
		// window is focused, so idle CPU usage is close to zero. Focus tracking
		// and keyboard mapping refreshes are delegated to FocusTracker.
		void RunPolling()
		{
			FocusTracker focus;
			if (!focus) return;

			Display* display = focus;
			const int fd = XConnectionNumber(display);

			KeyCode modifiers[ModCount][2] = {};
			ResolveModifiers(display, modifiers);

			const pid_t pid = getpid();
			bool focused = focus.Update(pid);
			char previous[32] = {};
			if (focused) XQueryKeymap(display, previous);

			while (!m_stop.load(std::memory_order_acquire)) {
				// PropertyChangeMask on the root window delivers _NET_ACTIVE_WINDOW
				// changes, so focus switches wake the thread immediately; otherwise
				// FocusTracker re-checks focus at most once a second.
				timeval timeout{ 0, focused ? 15000 : 200000 };
				fd_set set;
				FD_ZERO(&set);
				FD_SET(fd, &set);
				select(fd + 1, &set, nullptr, nullptr, &timeout);
				if (m_stop.load(std::memory_order_acquire)) break;

				if (focus.ConsumeMappingChange()) {
					// Keyboard mapping changed (layout or keyboard added).
					ResolveModifiers(display, modifiers);
					ResolveKeycodes(display);
				}

				bool now = focus.Focused(pid);
				if (now != focused) {
					focused = now;
					std::memset(previous, 0, sizeof(previous));
					if (focused) XQueryKeymap(display, previous);
					continue;
				}
				if (!focused) continue;

				char current[32] = {};
				XQueryKeymap(display, current);

				bool pressed = false;
				for (size_t i = 0; i < sizeof(current) && !pressed; ++i) {
					if (current[i] & ~previous[i]) pressed = true;
				}

				if (pressed) {
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

		void Run()
		{
			Display* ctrl = nullptr;
			XRecordContext context = 0;
			{
				std::lock_guard<std::mutex> lock(m_recordMutex);
				ctrl = m_recordCtrl;
				context = m_recordContext;
			}

			if (ctrl && context
				&& !m_stop.load(std::memory_order_acquire)
				&& RunRecord(ctrl, context)) {
				return;
			}

			if (m_stop.load(std::memory_order_acquire)) return;
			RunPolling();
		}

	public:
		~LinuxKeyboardHook()
		{
			Stop();
		}

		void Stop()
		{
			std::thread thread;
			Display* ctrl = nullptr;
			XRecordContext context = 0;
			{
				std::lock_guard<std::mutex> lock(m_recordMutex);
				thread = std::move(m_thread);
				ctrl = m_recordCtrl;
				context = m_recordContext;
			}

			if (thread.joinable()) {
				m_stop.store(true, std::memory_order_release);
				// Unblocks XRecordEnableContext; the worker never frees the
				// context itself.
				if (ctrl && context) XRecordDisableContext(ctrl, context);
				thread.join();
			}

			if (ctrl) {
				if (context) XRecordFreeContext(ctrl, context);
				XCloseDisplay(ctrl);
			}
			{
				std::lock_guard<std::mutex> lock(m_recordMutex);
				m_recordCtrl = nullptr;
				m_recordContext = 0;
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
			// thread opens its own ones.
			XDisplay display;
			if (!display) {
				m_hotkeys.clear();
				return "Keyboard hook error: cannot open X11 display";
			}

			std::string unresolved;
			for (auto it = m_hotkeys.begin(); it != m_hotkeys.end();) {
				it->keycode = FindKeycode(display, it->keysym);
				if (!it->keycode) {
					const char* name = XKeysymToString(it->keysym);
					unresolved += unresolved.empty() ? "" : ", ";
					unresolved += name ? name : "unknown";
					it = m_hotkeys.erase(it);
				}
				else ++it;
			}

			if (m_hotkeys.empty()) {
				return "Keyboard hook error: unsupported keys"
					+ (unresolved.empty() ? "" : (": " + unresolved));
			}

			// The control connection hosts the record context; Stop() disables
			// the context through it from the calling thread.
			Display* ctrl = XOpenDisplay(nullptr);
			if (ctrl) {
				int major = 0, minor = 0;
				if (XRecordQueryVersion(ctrl, &major, &minor)) {
					XRecordRange* range = XRecordAllocRange();
					if (range) {
						range->device_events.first = KeyPress;
						range->device_events.last = KeyRelease;
						XRecordClientSpec spec = XRecordAllClients;
						XRecordContext context = XRecordCreateContext(ctrl, 0, &spec, 1, &range, 1);
						XFree(range);
						if (context) {
							std::lock_guard<std::mutex> lock(m_recordMutex);
							m_recordCtrl = ctrl;
							m_recordContext = context;
						}
					}
				}
			}

			m_addin = addin;
			m_stop.store(false, std::memory_order_release);

			try {
				m_thread = std::thread(&LinuxKeyboardHook::Run, this);
			}
			catch (const std::exception& e) {
				m_addin = nullptr;
				m_hotkeys.clear();
				if (ctrl) {
					{
						std::lock_guard<std::mutex> lock(m_recordMutex);
						if (m_recordContext) XRecordFreeContext(ctrl, m_recordContext);
						m_recordCtrl = nullptr;
						m_recordContext = 0;
					}
					XCloseDisplay(ctrl);
				}
				return std::string("Keyboard hook error: ") + e.what();
			}
			return {};
		}

		// Called from the XRecord callback for every recorded key event.
		void OnRecordKey(RecordState* state, bool press, KeyCode code, Time time)
		{
			if (press) {
				// Skip auto-repeat: the key is already tracked as down, or the
				// release-press pair is too recent for a human press.
				bool repeat = state->down[code]
					|| (state->lastRelease[code] && time - state->lastRelease[code] < 25);
				state->down[code] = true;
				if (repeat) return;
			}
			else {
				state->down[code] = false;
				state->lastRelease[code] = time;
				return;
			}

			// Key codes may change when the layout or the keyboard changes. The
			// query connection receives MappingNotify on such changes; the
			// periodic rebinding below stays as a fallback for servers that
			// do not deliver it.
			if (state->focus->ConsumeMappingChange()) {
				ResolveKeycodes(state->query);
				ResolveModifiers(state->query, state->modifiers);
			}
			if (time - state->lastRebind > 10000) {
				state->lastRebind = time;
				ResolveKeycodes(state->query);
				ResolveModifiers(state->query, state->modifiers);
			}

			if (!state->focus->Focused(state->pid)) return;

			bool state_mod[ModCount];
			for (int i = 0; i < ModCount; ++i) {
				state_mod[i] = state->down[state->modifiers[i][0]] || state->down[state->modifiers[i][1]];
			}

			for (const auto& hotkey : m_hotkeys) {
				if (hotkey.keycode != code) continue;
				if (hotkey.ctrl != state_mod[ModCtrl] || hotkey.alt != state_mod[ModAlt]
					|| hotkey.shift != state_mod[ModShift] || hotkey.meta != state_mod[ModMeta]) continue;

				if (m_addin) m_addin->ExternalEvent(hotkey.message, hotkey.data);
				return;
			}
		}
	};

	// XRecord callback. Runs on the worker thread between replies of the data
	// connection; the query connection may be used here, the data one may not.
	void RecordCallback(XPointer closure, XRecordInterceptData* data)
	{
		auto* state = (RecordState*)closure;
		if (data->category == XRecordFromServer && data->data_len > 0) {
			const uint8_t type = ((const uint8_t*)data->data)[0];
			if (type == KeyPress || type == KeyRelease) {
				const uint8_t code = ((const uint8_t*)data->data)[1];
				if (code) {
					state->hook->OnRecordKey(state, type == KeyPress, (KeyCode)code, data->server_time);
				}
			}
		}
		XRecordFreeData(data);
	}

	LinuxKeyboardHook& GetHook()
	{
		static LinuxKeyboardHook hook;
		return hook;
	}

} // namespace

std::string KeyboardHook::Hook(AddInNative* addin, const std::u16string& msg)
{
	auto& hook = GetHook();
	if (!addin) {
		hook.Stop();
		return {};
	}
	return hook.Start(addin, msg);
}

#endif //_WINDOWS
