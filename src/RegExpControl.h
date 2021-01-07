#ifndef __RegExpComponent_H__
#define __RegExpComponent_H__

#ifdef USE_BOOST

#include "stdafx.h"
#include "AddInNative.h"

class RegExpComponent:
    public AddInNative
{
private:
    static std::vector<std::u16string> names;
    RegExpComponent();
private:
    std::wstring Replace(const std::wstring& source, const std::wstring& search, const std::wstring& replace);
    std::wstring Split(const std::wstring& source, const std::wstring& search);
};

#endif //USE_BOOST

#endif //__RegExpComponent_H__