cmake_minimum_required(VERSION 3.10)

project(VanessaExt)
set(TARGET VanessaExt)

add_library(${TARGET} SHARED
	version.h
	src/AddInNative.cpp
	src/AddInNative.def
	src/AddInNative.h
	src/AddInNative.rc
	src/BaseHelper.h
	src/ClickEffect.cpp
	src/ClickEffect.h 	
	src/ClipboardControl.cpp
	src/ClipboardControl.h
	src/ClipboardManager.cpp 
	src/ClipboardManager.h 
	src/FileFinder.cpp
	src/FileFinder.h
	src/GitManager.cpp
	src/GitManager.h
	src/ImageFinder.cpp
	src/ImageFinder.h
	src/ImageHelper.cpp
	src/ImageHelper.h
	src/ProcessControl.cpp
	src/ProcessControl.h
	src/ProcessManager.cpp
	src/ProcessManager.h
	src/SoundEffect.cpp
	src/SoundEffect.h 	
	src/ScreenManager.cpp 
	src/ScreenManager.h 
	src/WebSocket.h
	src/WebSocket.cpp
	src/WindowsControl.cpp
	src/WindowsControl.h
	src/WindowsManager.cpp
	src/WindowsManager.h
	src/screenshot.cpp
	src/screenshot.h
	src/xcb_clip.cpp
	src/xcb_clip.h
	src/json.hpp
	src/stdafx.h)

target_compile_definitions(${TARGET} PRIVATE UNICODE _UNICODE)
target_include_directories(${TARGET} PRIVATE include)
set (LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set (EXECUTABLE_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH})
set (CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})

if (UNIX)
	set(MyPrefix "")
	if (APPLE)
		set(MySuffix1 "Mac")
	else(APPLE)
		set(MySuffix1 "Lin")
	endif(APPLE)
	if (TARGET_PLATFORM_32)
		set(MySuffix2 "32")
	else()
		set(MySuffix2 "64")
	endif()
else(UNIX)
	if (NOT MSVC)
		message(FATAL_ERROR "Must be compiled with MSVC on Windows")
	endif(NOT MSVC)
	set(MyPrefix "lib")
	set(MySuffix1 "Win")
endif(UNIX)

set_target_properties( ${PROJECT_NAME} PROPERTIES
	   OUTPUT_NAME ${MyPrefix}${PROJECT_NAME}${MySuffix1}${MySuffix2} 
	   POSITION_INDEPENDENT_CODE ON 
	   CXX_STANDARD_REQUIRED ON
	   CXX_STANDARD 17
   )

if (UNIX)
	if (TARGET_PLATFORM_32)
		set(CMAKE_LIBRARY_PATH "/usr/lib/i386-linux-gnu" CACHE PATH "<desc>")
		SET(ARCH_FLAG "-m32")
	else()
		SET(ARCH_FLAG "-m64")
	endif()
	foreach(flag
		CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_C_FLAGS CMAKE_CXX_FLAGS )
		if(NOT ${flag} MATCHES ${ARCH_FLAG})
			set(${flag} ${${flag}} ${ARCH_FLAG} CACHE "STRING" "Linker flags" FORCE)
		endif()
	endforeach()
	include(FindX11)
	find_package(X11 COMPONENTS Xtst)
	target_link_libraries(${PROJECT_NAME} ${X11_XTest_LIB})
	target_link_libraries(${PROJECT_NAME} -static-libgcc -static-libstdc++)
	target_link_libraries(${TARGET} opencv_core opencv_imgcodecs opencv_imgproc libpng)
else(UNIX)
	add_definitions(/MT)
	set(CMAKE_SUPPRESS_REGENERATION 1)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
	target_compile_definitions(${TARGET} PRIVATE _WINDOWS
			_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
	target_compile_options(${TARGET} PRIVATE /utf-8)
	target_link_libraries(${TARGET} opencv_core450.lib opencv_imgcodecs450.lib opencv_imgproc450.lib libpng.lib zlib.lib)
endif(UNIX)

target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/opencv-x${MySuffix2}")
target_link_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/opencv-x${MySuffix2}")

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME ON) 
set(Boost_NO_SYSTEM_PATHS TRUE) 

if (Boost_NO_SYSTEM_PATHS)
set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/boost_1_74_0")
set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/stage/lib")
endif (Boost_NO_SYSTEM_PATHS)

find_package(Boost REQUIRED regex date_time)

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})

message("========================")
message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message("Boost_LIBRARIES: ${Boost_LIBRARIES}")
message("========================")

target_include_directories(${TARGET} PRIVATE libgit2-1.0.1/include)
set(EMBED_SSH_PATH "${CMAKE_SOURCE_DIR}/libssh2-1.9.0" CACHE PATH "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR OFF CACHE BOOL "" FORCE)
set(STATIC_CRT ON CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_subdirectory(./libgit2-1.0.1)
target_link_libraries(${TARGET} git2)
