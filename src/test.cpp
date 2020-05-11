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

int main() {
    setlocale(LC_ALL, "");
    std::wstring json;

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

    return 0;
}
