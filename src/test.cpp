#include <iostream>
#include <cstdio>
#include <string>

unsigned int FindTestClient(int port, std::wstring &json);

int main() {
    setlocale(LC_ALL, "");

    std::wstring json;
    FindTestClient(48000, json);
    std::wcout << std::endl << json << std::endl << std::endl;

    return 0;
}
