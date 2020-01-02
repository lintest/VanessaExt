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
*/
    tVariant pVarTrue;
    pVarTrue.intVal = 1;
    pVarTrue.vt = VTYPE_BOOL;

    json = ProcessManager::GetProcessList(&pVarTrue, 1);
    std::wcout << std::endl << json << std::endl << std::endl;

    tVariant pVarClient;
    pVarTrue.intVal = 48000;
    pVarTrue.vt = VTYPE_I4;

    json = ProcessManager::FindTestClient(&pVarTrue, 1);
    std::wstring text = json;
    std::wcout << std::endl << json << std::endl << std::endl;
/*
    json = WindowManager().GetWindowList(NULL, 0);
    std::wcout << std::endl << json << std::endl << std::endl;
*/
    return 0;
}
