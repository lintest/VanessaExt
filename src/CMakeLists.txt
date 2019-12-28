# Cmake script for project AddInNative
# autor: 1C

CMAKE_MINIMUM_REQUIRED(VERSION 3.0 FATAL_ERROR)

foreach(policy CMP0068)
  if(POLICY ${policy})
    cmake_policy(SET ${policy} OLD)
  endif()
endforeach()

PROJECT(1cWinCtrl)

SET(AddInNative_SRC 
    AddInNative.cpp
    AddInNative.h
    ProcMngr.cpp
    ProcMngr.h
    WinCtrl.cpp
    WinCtrl.h
    json.hpp
    dllmain.cpp
    stdafx.cpp
    stdafx.h
    )

include_directories(${CMAKE_SOURCE_DIR}/include)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

SET (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR})
SET(AddInDef_SRC)

if (UNIX)
    if (APPLE)
        ADD_DEFINITIONS(-D__APPLE__ )
    endif()
else()
    SET(AddInDef_SRC AddInNative.def)
    add_definitions(-DUNICODE -DWIN32 -D_USRDLL -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -DADDINCPP_EXPORTS)
    IF(TARGET_PLATFORM_32)
        add_definitions(-D_USE_32BIT_TIME_T )
    ENDIF()
endif()
if(APPLE)
    set(BNDL ${PROJECT_NAME}Bndl)

    add_library(${BNDL} MODULE ${AddInNative_SRC})

    set_target_properties(${BNDL} PROPERTIES BUNDLE TRUE)
    set_target_properties( ${BNDL} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
    SET(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME}")
    string(TIMESTAMP YEAR "%Y")
    SET(MACOSX_BUNDLE_COPYRIGHT "Copyright ${YEAR} ${MYCOMPANY}.")
    TARGET_LINK_LIBRARIES(${BNDL} "-framework CoreFoundation")
    set_target_properties(${BNDL} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/MacOSXBundleInfo.plist.in)
endif()

add_library(${PROJECT_NAME} SHARED ${AddInNative_SRC} ${AddInDef_SRC})

set_target_properties( ${PROJECT_NAME} PROPERTIES
       OUTPUT_NAME ${PROJECT_NAME}${MySuffix1}${MySuffix2} 
       POSITION_INDEPENDENT_CODE ON 
       CXX_STANDARD 11
       CXX_STANDARD_REQUIRED ON
   )
