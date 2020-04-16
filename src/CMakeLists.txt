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
    WebSocket.h
    WebSocket.cpp
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

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME ON) 
set(Boost_NO_SYSTEM_PATHS TRUE) 

if (Boost_NO_SYSTEM_PATHS)
  set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../boost_1_72_0")
  set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
  set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/stage/lib")
endif (Boost_NO_SYSTEM_PATHS)

find_package(Boost REQUIRED regex date_time system filesystem thread graph program_options)

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

message("========================")
message(${Boost_INCLUDE_DIRS})
message(${Boost_LIBRARIES})
message("========================")
