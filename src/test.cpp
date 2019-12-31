#include <iostream>
#include <cstdio>
#include <string>
#include <types.h>
#include "ProcMngr.h"
#include "WinCtrl.h"

unsigned int FindTestClient(int port, std::wstring &json);

int main() {
    setlocale(LC_ALL, "");

    tVariant paParam;
    paParam.intVal = 44040523;
    paParam.vt = VTYPE_I4;
    std::wstring json = WindowsControl().GetChildWindows(&paParam, 1);
    std::wcout << std::endl << json << std::endl << std::endl;

    return 0;
}
