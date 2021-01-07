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
    int64_t value;
private:
    std::u16string text;
    std::u16string getTestString();
    void setTestString(const std::u16string &text);
};

#endif //USE_BOOST

#endif //__RegExpComponent_H__