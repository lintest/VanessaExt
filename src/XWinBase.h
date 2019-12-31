#ifndef __XWINBASE_H__
#define __XWINBASE_H__

#include "stdafx.h"

#ifdef __linux__

#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

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

public:
    WindowHelper() {
        display = XOpenDisplay(NULL);
	}
    ~WindowHelper() {
        if (display) XCloseDisplay(display);
    }

	operator std::wstring() {
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
        if (GetProperty(window, XA_STRING, "WM_NAME", VXX(&buffer), NULL)) return S(buffer);
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

#endif //__linux__

#endif //__XWINBASE_H__