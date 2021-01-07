#ifndef __RegExpComponent_H__
#define __RegExpComponent_H__

#ifdef USE_BOOST

#include "AddInNative.h"

class RegExpComponent:
    public AddInNative
{
private:
    static std::vector<std::u16string> names;
    RegExpComponent();
private:
    std::wstring Replace(const std::wstring& source, const std::wstring& search, const std::wstring& replace);
};

#endif //USE_BOOST

#endif //__RegExpComponent_H__