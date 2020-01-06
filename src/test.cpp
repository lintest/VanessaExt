#include <iostream>
#include <cstdio>
#include <string>
#include <types.h>
#include "ProcMngr.h"
#include "ScreenMngr.h"
#include "WindowMngr.h"

int main() {
    setlocale(LC_ALL, "");
    std::wstring json;

/*
    tVariant paParam;
    paParam.intVal = 44040523;
    paParam.vt = VTYPE_I4;

    json = ScreenManager::GetDisplayList(NULL, 0);
    std::wcout << std::endl << json << std::endl << std::endl;

    tVariant pVarTrue;
    pVarTrue.intVal = 1;
    pVarTrue.vt = VTYPE_BOOL;

    json = ProcessManager::GetProcessList(&pVarTrue, 1);
    std::wcout << std::endl << json << std::endl << std::endl;

    tVariant pVarClient;
    pVarClient.intVal = 48000;
    pVarClient.vt = VTYPE_I4;

    json = ProcessManager::FindTestClient(&pVarClient, 1);
    std::wstring text = json;
    std::wcout << std::endl << json << std::endl << std::endl;

    tVariant pVarProc;
    pVarProc.intVal = 4792;
    pVarProc.vt = VTYPE_I4;

    json = WindowManager().GetWindowList(&pVarProc, 1);
    std::wcout << std::endl << json << std::endl << std::endl;

    json = ScreenManager::GetScreenList();
    std::wcout << std::endl << json << std::endl << std::endl;

    json = ScreenManager::GetDisplayList(NULL, 0);
    std::wcout << std::endl << json << std::endl << std::endl;

    json = WindowManager::GetWindowSize(NULL, 0);
    std::wcout << std::endl << json << std::endl << std::endl;
*/    

    tVariant pVarActive;
    pVarActive.intVal = (int)WindowManager::ActiveWindow();
    pVarActive.vt = VTYPE_I4;

    json = ScreenManager::GetScreenInfo();
    std::wcout << std::endl << json << std::endl << std::endl;

    json = ScreenManager::GetDisplayList(&pVarActive, 1);
    std::wcout << std::endl << json << std::endl << std::endl;

    json = ScreenManager::GetDisplayInfo(&pVarActive, 1);
    std::wcout << std::endl << json << std::endl << std::endl;

    json = WindowManager::GetWindowInfo(NULL, 0);
    std::wcout << std::endl << json << std::endl << std::endl;

    return 0;
}
