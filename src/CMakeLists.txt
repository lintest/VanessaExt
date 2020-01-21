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
    AddInBase.cpp
    AddInBase.h
    ClipboardControl.cpp
    ClipboardControl.h
    WindowsControl.cpp
    WindowsControl.h
    ImageHelper.cpp
    ImageHelper.h
    ProcMngr.cpp
    ProcMngr.h
    ClipMngr.cpp 
    ClipMngr.h 
    ScreenMngr.cpp 
    ScreenMngr.h 
    WindowMngr.cpp
    WindowMngr.h
    screenshot.cpp
    screenshot.h
	xcb_clip.cpp
	xcb_clip.h
    json.hpp
    dllmain.cpp
    stdafx.cpp
    stdafx.h
)

include_directories(${CMAKE_SOURCE_DIR}/include)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")

SET (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${CMAKE_SOURCE_DIR})
SET(AddInDef_SRC)

if (WINDOWS)
    SET(AddInDef_SRC AddInNative.def)
    add_definitions(-DUNICODE -D_WINDOWS -DWIN32 -D_USRDLL -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -DADDINCPP_EXPORTS)
    IF(TARGET_PLATFORM_32)
        add_definitions(-D_USE_32BIT_TIME_T )
    ENDIF()
endif()

add_library(${PROJECT_NAME} SHARED ${AddInNative_SRC} ${AddInDef_SRC})

set_target_properties( ${PROJECT_NAME} PROPERTIES
       OUTPUT_NAME ${PROJECT_NAME}${MySuffix1}${MySuffix2} 
       POSITION_INDEPENDENT_CODE ON 
       CXX_STANDARD 11
       CXX_STANDARD_REQUIRED ON
   )

if (WINDOWS)
    target_link_libraries(${PROJECT_NAME} gdiplus)
endif()
