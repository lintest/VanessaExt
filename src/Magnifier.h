#pragma once

#ifdef _WINDOWS

class Magnifier
{
public:
    static void Show(int x, int y, int w, int h, double z);
    static void Hide();
};

#endif //_WINDOWS
