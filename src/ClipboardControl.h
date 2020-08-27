#ifndef __CLIPBOARDCONTROL_H__
#define __CLIPBOARDCONTROL_H__

#include "stdafx.h"
#include "BaseHelper.h"

class ClipboardControl : public BaseHelper
{
private:
    static std::vector<std::u16string> names;
    ClipboardControl();
};

#endif //__CLIPBOARDCONTROL_H__
