#include <iostream>
#include <cstdio>
#include <string>
#include "ProcMngr.h"

unsigned int FindTestClient(int port, std::wstring &json);

int main() {
    setlocale(LC_ALL, "");

    std::wstring json = ProcessManager::GetProcessList(NULL, 0);
    std::wcout << std::endl << json << std::endl << std::endl;

    return 0;
}
