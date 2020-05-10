#include <iostream>
#include <cstdio>
#include <string>
#include <types.h>
#include "ClipMngr.h"
#include "ProcMngr.h"
#include "ScreenMngr.h"
#include "WindowMngr.h"
#include "json_ext.h"
#include "xcb_clip.h"
#include <X11/Xlib.h>

int main() {
    setlocale(LC_ALL, "");
    std::wstring json;

/*
    tVariant paParam;
    paParam.intVal = 44040523;
    paParam.vt = VTYPE_I4;


    tVariant pVarClient;
    pVarClient.intVal = 48000;
    pVarClient.vt = VTYPE_I4;

    json = ProcessManager::FindTestClient(&pVarClient, 1);
    std::wstring text = json;
    std::wcout << std::endl << json << std::endl << std::endl;
*/

    std::wcout << L"XOpenDisplay";
    Display* display = XOpenDisplay(NULL);
    std::wcout << std::endl << display << std::endl << std::endl;

    std::wcout << L"DefaultRootWindow";
    Window root = DefaultRootWindow(display);
    std::wcout << std::endl << root << std::endl << std::endl;

    if (display) XCloseDisplay(display);

    tVariant pVarProc;
    pVarProc.intVal = 0;
    pVarProc.vt = VTYPE_I4;
    std::wcout << L"GetWindowList";
    json = WindowManager().GetWindowList(&pVarProc, 1);
    std::wcout << std::endl << json << std::endl << std::endl;

    std::wcout << L"GetScreenList";
    json = ScreenManager::GetScreenList();
    std::wcout << std::endl << json << std::endl << std::endl;

    std::wcout << L"GetDisplayList";
    json = ScreenManager::GetDisplayList(NULL, 0);
    std::wcout << std::endl << json << std::endl << std::endl;

    tVariant pVarTrue;
    pVarTrue.intVal = 1;
    pVarTrue.vt = VTYPE_BOOL;
    std::wcout << L"GetProcessList";
    json = ProcessManager::GetProcessList(&pVarTrue, 1);
    std::wcout << std::endl << json << std::endl << std::endl;

    std::wcout << L"PrintWindow";
    unsigned int window = 0;
    if (!json.empty()) {
        auto j = nlohmann::json::parse(json);
        if (j.is_array() && j.size() == 1) {
            auto jj = j[0];
            window = jj["Window"];
        }
    }
    std::wcout << std::endl << window;
    ScreenManager::PrintWindow(window);

    return 0;
}
