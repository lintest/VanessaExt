cmake_minimum_required(VERSION 3.10)

project(VanessaExt)
set(TARGET VanessaExt)

option(WITH_BOOST "Use boost library" ON)
option(WITH_GHERKIN "Gherkin language parser" ON)
option(WITH_LIBGIT2 "Use library libgit2" ON)
option(WITH_OPENCV "Use OpenCV library" ON)

if (WITH_GHERKIN)
	add_definitions(-DUSE_GHERKIN)

	ADD_CUSTOM_TARGET(GherkinLexer echo Creating GherkinLexer)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/include)

	if (UNIX)
		set(REFLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/bin/reflex" CACHE PATH "Reflex executable")
	else(UNIX)
		set(REFLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/bin/win64/reflex.exe" CACHE PATH "Reflex executable")
	endif(WIN32)

	add_custom_command(TARGET GherkinLexer
		COMMAND ${REFLEX_EXECUTABLE} 
		ARGS --unicode --lexer=GherkinLexer 
			--outfile=gherkin.lex.cpp --header-file=gherkin.lex.h
			${CMAKE_CURRENT_SOURCE_DIR}/src/gherkin.l
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/gherkin.l
		OUTPUTS gherkin.lex.cpp gherkin.lex.h
		VERBATIM
	)

	set(GherkinLexer_SRCS
		RE-flex/lib/convert.cpp
		RE-flex/lib/debug.cpp
		RE-flex/lib/error.cpp
		RE-flex/lib/input.cpp
		RE-flex/lib/matcher.cpp
		RE-flex/lib/pattern.cpp
		RE-flex/lib/posix.cpp
		RE-flex/lib/unicode.cpp
		RE-flex/lib/utf8.cpp
		RE-flex/unicode/block_scripts.cpp
		RE-flex/unicode/language_scripts.cpp
		RE-flex/unicode/letter_scripts.cpp
		src/GherkinParser.cpp
		gherkin.lex.cpp
		gherkin.lex.h
		src/gherkin.cpp
		src/gherkin.h
		src/json.hpp
	)

	set_source_files_properties( 
		gherkin.lex.cpp
		gherkin.lex.h
		GENERATED
	)
endif (WITH_GHERKIN)

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
	src/DesktopManager.cpp
	src/DesktopManager.h
	src/FileFinder.cpp
	src/FileFinder.h
	src/GitManager.cpp
	src/GitManager.h
	src/ImageFinder.cpp
	src/ImageFinder.h
	src/ImageHelper.cpp
	src/ImageHelper.h
	src/Magnifier.cpp
	src/Magnifier.h
	src/ProcessControl.cpp
	src/ProcessControl.h
	src/ProcessManager.cpp
	src/ProcessManager.h
	src/RegExpControl.cpp
	src/RegExpControl.h
	src/SoundEffect.cpp
	src/SoundEffect.h 	
	src/ScreenManager.cpp 
	src/ScreenManager.h 
	src/VideoPainter.cpp
	src/VideoPainter.h
	src/WebSocket.h
	src/WebSocket.cpp
	src/WindowsControl.cpp
	src/WindowsControl.h
	src/WindowsManager.cpp
	src/WindowsManager.h
	src/WinUIAuto.cpp
	src/WinUIAuto.h
	src/X11Screenshot.cpp
	src/X11Screenshot.h
	${GherkinLexer_SRCS}
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
	   PREFIX ""
	   OUTPUT_NAME ${PROJECT_NAME}${MySuffix1}${MySuffix2} 
	   POSITION_INDEPENDENT_CODE ON 
	   CXX_STANDARD_REQUIRED ON
	   CXX_STANDARD 17
   )

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR OFF CACHE BOOL "" FORCE)
set(STATIC_CRT ON CACHE BOOL "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
	target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/3rdparty/clip-1.3")
	add_subdirectory("${CMAKE_SOURCE_DIR}/3rdparty/clip-1.3")
	target_link_libraries(${TARGET} clip)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
else(UNIX)
	add_definitions(/MT)
	set(CMAKE_SUPPRESS_REGENERATION 1)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
	target_compile_definitions(${TARGET} PRIVATE _WINDOWS
			_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
	target_compile_options(${TARGET} PRIVATE /utf-8)
endif(UNIX)

if (WITH_OPENCV)
	MESSAGE(STATUS "WITH_OPENCV")
	add_definitions(-DUSE_OPENCV)
	target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/opencv-x${MySuffix2}")
	if (UNIX)
		target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/opencv-4.5.2/3rdparty/libpng")
		target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/opencv-4.5.2/3rdparty/zlib")	
		find_package(OpenCV REQUIRED PATHS "${CMAKE_SOURCE_DIR}/opencv-x${MySuffix2}" NO_DEFAULT_PATH)
		target_link_libraries(${TARGET} opencv_core opencv_imgcodecs opencv_imgproc libpng zlib)
	else(UNIX)
		target_link_libraries(${TARGET} opencv_core452.lib opencv_imgcodecs452.lib opencv_imgproc452.lib libpng.lib zlib.lib)
		target_link_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/opencv-x${MySuffix2}")
	endif(UNIX)
endif (WITH_OPENCV)

if (WITH_BOOST)
	MESSAGE(STATUS "WITH_BOOST")
	add_definitions(-DUSE_BOOST)
	set(Boost_USE_STATIC_LIBS ON) 
	set(Boost_USE_MULTITHREADED ON)  
	set(Boost_USE_STATIC_RUNTIME ON) 
	set(Boost_NO_SYSTEM_PATHS TRUE) 
	set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/boost_1_76_0")
	set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
	set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/stage/lib")
	find_package(Boost REQUIRED regex date_time filesystem)
	include_directories(${Boost_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif (WITH_BOOST)

if (WITH_LIBGIT2)
	MESSAGE(STATUS "WITH_LIBGIT2")
	add_definitions(-DUSE_LIBGIT2)
	target_include_directories(${TARGET} PRIVATE libgit2-1.1.0/include)
	set(EMBED_SSH_PATH "${CMAKE_SOURCE_DIR}/libssh2-1.9.0" CACHE PATH "" FORCE)
	add_subdirectory(./libgit2-1.1.0)
	target_link_libraries(${TARGET} git2)
endif (WITH_LIBGIT2)

if (WITH_GHERKIN)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
	include_directories(${VanessaExt_BINARY_DIR})
	add_dependencies(${TARGET} GherkinLexer)
endif (WITH_GHERKIN)