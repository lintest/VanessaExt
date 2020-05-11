#ifndef __XWINBASE_H__
#define __XWINBASE_H__

#include "stdafx.h"
#include "json_ext.h"

#ifndef _WINDOWS

#include <fstream>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#define p_verbose(...) if (envir_verbose) { \
    fprintf(stderr, __VA_ARGS__); \
}

static bool envir_verbose = false;

using namespace std;

#define VXX(T) reinterpret_cast<void**>(T)

class WindowHelper
{
protected:
    JSON json;
    Display* display = NULL;

    class Rect 
    {
    protected:
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
        int min(int a, int b) { return a < b ? a : b; }
        int max(int a, int b) { return a > b ? a : b; }
    public:
        Rect() 
            : left(0), top(0), right(0), bottom(0) {}

        Rect(XRRMonitorInfo* i)
            : left(i->x), top(i->y), right(i->x + i->width), bottom(i->y + i->height) {}

        Rect(Display* display, Window window)
            : left(0), top(0), right(0), bottom(0)
        {
            if (!window) return;
            Window junkroot;
            int x, y, junkx, junky;
            unsigned int w, h, bw, depth;
            Status status = XGetGeometry(display, window, &junkroot, &junkx, &junky, &w, &h, &bw, &depth);
            if (!status) return;
            Bool ok = XTranslateCoordinates(display, window, junkroot, junkx, junky, &x, &y, &junkroot);
            if (!ok) return;
            left = x;
            top = y;
            right = x + w;
            bottom = y + h;
        }

        int operator *(const Rect &r) {
            int width = min(right, r.right) - max(left, r.left);
            int height = min(bottom, r.bottom) - max(top, r.top);
            return max(width, 0) * max(height, 0);
        }
    };

public:
    WindowHelper() {
        display = XOpenDisplay(NULL);
	}
    virtual ~WindowHelper() {
        if (display) XCloseDisplay(display);
    }

    operator bool() {
        return !json.empty();
    }

	operator std::wstring() const {
		return json;
	}

protected:
    bool GetProperty(Window window, Atom xa_prop_type, const char *prop_name, void **ret_prop, unsigned long *size) {
        int ret_format;
        Atom xa_prop_name, xa_ret_type;
        unsigned long ret_bytes_after;
        xa_prop_name = XInternAtom(display, prop_name, False);
        if (XGetWindowProperty(display, window, xa_prop_name, 0, ~0L, False,
                xa_prop_type, &xa_ret_type, &ret_format,
                size, &ret_bytes_after, (unsigned char**)ret_prop) != Success) {
            p_verbose("Cannot get %s property.\n", prop_name);
            return false;
        }
        if (xa_ret_type != xa_prop_type) {
            p_verbose("Invalid type of %s property.\n", prop_name);
            XFree(*ret_prop);
            return false;
        }
        return true;
    }

    int SendMessage(Window window, char *msg, 
        unsigned long data0 = 0, unsigned long data1 = 0, 
        unsigned long data2 = 0, unsigned long data3 = 0,
        unsigned long data4 = 0) 
    {
        XEvent event;
        long mask = SubstructureRedirectMask | SubstructureNotifyMask;

        event.xclient.type = ClientMessage;
        event.xclient.serial = 0;
        event.xclient.send_event = True;
        event.xclient.message_type = XInternAtom(display, msg, False);
        event.xclient.window = window;
        event.xclient.format = 32;
        event.xclient.data.l[0] = data0;
        event.xclient.data.l[1] = data1;
        event.xclient.data.l[2] = data2;
        event.xclient.data.l[3] = data3;
        event.xclient.data.l[4] = data4;

        if (XSendEvent(display, DefaultRootWindow(display), False, mask, &event)) {
            return EXIT_SUCCESS;
        }
        else {
            fprintf(stderr, "Cannot send %s event.\n", msg);
            return EXIT_FAILURE;
        }
    }

    unsigned long GetWindowPid(Window window) {
		unsigned long size, *pid = NULL;
		GetProperty(window, XA_CARDINAL, "_NET_WM_PID", VXX(&pid), &size);
		unsigned long result = pid ? *pid : 0;
		XFree(pid);
		return result;
    }

    Window GetWindowOwner(Window window) {
        Window parent = 0;
        Status status = XGetTransientForHint(display, window, &parent);
        return status ? parent : 0;
    }

    std::string S(char *buffer) {
        if (!buffer) return {};
        std::string result(buffer);
		XFree(buffer);
        return result;
    }

    std::string GetWindowTitle(Window window) {
        unsigned long size;
        char *buffer = NULL;
        if (GetProperty(window, XInternAtom(display, "UTF8_STRING", False), "_NET_WM_NAME", VXX(&buffer), &size)) return S(buffer);
        if (GetProperty(window, XA_STRING, "WM_NAME", VXX(&buffer), &size)) return S(buffer);
        return {};
    }

    std::string GetWindowClass(Window window) {
        unsigned long size;
        char *buffer = NULL;
        if (!GetProperty(window, XA_STRING, "WM_CLASS", VXX(&buffer), &size)) return {};
        char *p0 = strchr(buffer, '\0');
        if (buffer + size - 1 > p0) *(p0) = '.';
        return S(buffer);
    }

	bool IsMaximized(Window window) {
        bool max_horz = false, max_vert = false;
        Atom xa_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
		Atom xa_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);        
		unsigned long size, *states = NULL;
        if (!GetProperty(window, XA_ATOM, "_NET_WM_STATE", VXX(&states), &size)) return false;
        for (unsigned long i = 0; i < size; i++) {
            if (states[i] == xa_horz) max_horz = true;
            if (states[i] == xa_vert) max_vert = true;
        }
        return max_horz && max_vert;
    }


public:
    Window GetActiveWindow() {
        Window *buffer = NULL;
        unsigned long size;
        Window root = DefaultRootWindow(display);
        if (!GetProperty(root, XA_WINDOW, "_NET_ACTIVE_WINDOW", VXX(&buffer), &size)) return NULL;
        Window result = buffer ? *buffer : NULL;
        XFree(buffer);
        return result;
    }

    void SetActiveWindow(Window window) {
        SendMessage(window, "_NET_ACTIVE_WINDOW", 1, CurrentTime);
    }

     void Maximize(Window window, bool state) {
        SendMessage(window, "_NET_WM_STATE_HIDDEN", 0, CurrentTime);
        Atom prop1 = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        Atom prop2 = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
        SendMessage(window, "_NET_WM_STATE", state ? 1 : 2, prop1, prop2);
    }

    void Minimize(Window window) {
        SendMessage(window, "_NET_WM_STATE_HIDDEN", 1, CurrentTime);
		XLowerWindow(display, window);
    }

	void SetWindowPos(Window window, unsigned long x, unsigned long y) {
		XMoveWindow(display, window, x, y);
	}

	void SetWindowSize(Window window, unsigned long w, unsigned long h) {
		XResizeWindow(display, window, w, h);
	}
};

class WindowEnumerator : public WindowHelper
{
private:
    Window* m_windows = NULL;
    unsigned long m_count = 0;

protected:
    virtual bool EnumWindow(Window window) = 0;

public:
    WindowEnumerator() {
        Window root = DefaultRootWindow(display);
        if (GetProperty(root, XA_WINDOW, "_NET_CLIENT_LIST", VXX(&m_windows), &m_count)) return;
        if (GetProperty(root, XA_WINDOW, "_WIN_CLIENT_LIST", VXX(&m_windows), &m_count)) return;
        m_count = 0; // Cannot get client list properties.
    }

    std::wstring Enumerate() {
        for (int i = 0; i < m_count; i++) {
            if (!EnumWindow(m_windows[i])) break;
        }
        return *this;
    }

	virtual ~WindowEnumerator() {
        XFree(m_windows);
	}
};

#endif //_WINDOWS

#endif //__XWINBASE_H__
