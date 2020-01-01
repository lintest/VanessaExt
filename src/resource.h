#ifndef __RESOURCE_H__

#define IDR_POPUP_MENU                  4
#define IDI_TRAYICON                    5
#define IDD_ABOUT                       108
#define ID_POPUP_ENABLE                 40007
#define ID_POPUP_EXIT                   40009
#define ID_POPUP_ABOUT                  40011

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define VERSION_MAJOR               0
#define VERSION_MINOR               5
#define VERSION_REVISION            0
#define VERSION_BUILD               0

#define VER_FILE_DESCRIPTION_STR    "1C Window Control AddIn"
#define VER_FILE_VERSION            VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD
#define VER_FILE_VERSION_STR        STRINGIZE(VERSION_MAJOR)        \
                                    "." STRINGIZE(VERSION_MINOR)    \
                                    "." STRINGIZE(VERSION_REVISION) \
                                    "." STRINGIZE(VERSION_BUILD)    \

#define VER_PRODUCTNAME_STR         VER_FILENAME
#define VER_PRODUCT_VERSION         VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR     VER_FILE_VERSION_STR
#define VER_ORIGINAL_FILENAME_STR   VER_FILENAME VER_FILE_EXT
#define VER_INTERNAL_NAME_STR       VER_ORIGINAL_FILENAME_STR
#define VER_COPYRIGHT_STR           "Kandrashin Denis, 2020"

#ifdef _DEBUG
#define VER_VER_DEBUG             VS_FF_DEBUG
#else
#define VER_VER_DEBUG             0
#endif

#define VER_FILEOS                  VOS_NT_WINDOWS32
#define VER_FILEFLAGS               VER_VER_DEBUG
#define VER_FILETYPE                VFT_APP

#endif // __RESOURCE_H__
//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by AddInNative.rc

// ��������� ����������� �������� ��� ����� ��������
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        101
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif
