#pragma once
#include "stdafx.h"
#include "BaseHelper.h"
class ClipboardControl : public BaseHelper
{
#ifdef _WINDOWS
private:
    HWND hClipboardMonitor = NULL;
    void SetMonitoring(bool value);
    bool GetMonitoring();
public:
    void SendEvent();
#endif //_WINDOWS
private:
    static std::vector<std::u16string> names;
    ClipboardControl();
    virtual ~ClipboardControl();
};
