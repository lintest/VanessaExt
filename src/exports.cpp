#include "stdafx.h"
#include "AddInNative.h"

const WCHAR_T *GetClassNames() {
    static char16_t cls_names[] = u"WindowsControl";
    return reinterpret_cast<WCHAR_T *>(cls_names);
}

long GetClassObject(const WCHAR_T *clsName, IComponentBase **pInterface) {
    if (!*pInterface) {
        *pInterface = new CAddInNative;
        return (long) *pInterface;
    }
    return 0;
}

long DestroyObject(IComponentBase **pInterface) {
    if (!*pInterface) {
        return -1;
    }

    delete *pInterface;
    *pInterface = nullptr;
    return 0;
}

AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities) {
    return eAppCapabilitiesLast;
}
