#define VERSION_MAJOR               1
#define VERSION_MINOR               1
#define VERSION_REVISION            9
#define VERSION_BUILD               0

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define VER_FILE_VERSION            VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD
#define VER_FILE_VERSION_STR        STRINGIZE(VERSION_MAJOR)        \
                                    "." STRINGIZE(VERSION_MINOR)    \
                                    "." STRINGIZE(VERSION_REVISION) \
                                    "." STRINGIZE(VERSION_BUILD)    

#define VER_FILE_DESCRIPTION_STR    "Vanessa Automation AddIn for 1C"
#define VER_COPYRIGHT_STR           "Kandrashin Denis, 2020"
#define VER_FILENAME                "VanessaExt"
#define VER_FILE_EXT                ".dll" 
