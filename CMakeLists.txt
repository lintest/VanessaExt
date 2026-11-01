cmake_minimum_required(VERSION 3.13)

project(VanessaExt)
set(TARGET VanessaExt)

option(WITH_BOOST "Use boost library" ON)
option(WITH_GHERKIN "Gherkin language parser" ON)
option(WITH_LIBGIT2 "Use library libgit2" ON)
option(WITH_OPENCV "Use OpenCV library" ON)
option(VANESSAEXT_BUILD_TESTS "Build VanessaExt tests" OFF)

find_package(Threads REQUIRED)

if(WITH_GHERKIN)
	add_definitions(-DUSE_GHERKIN)

	ADD_CUSTOM_TARGET(GherkinLexer echo Creating GherkinLexer)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/include)

	if(UNIX)
		set(REFLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/bin/reflex" CACHE PATH "Reflex executable")
	else(UNIX)
		set(REFLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/RE-flex/bin/win64/reflex.exe" CACHE PATH "Reflex executable")
	endif(UNIX)

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
endif(WITH_GHERKIN)

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
	src/EducationShow.cpp
	src/EducationShow.h
	src/EventMonitor.cpp
	src/EventMonitor.h
	src/FileFinder.cpp
	src/FileFinder.h
	src/GitManager.cpp
	src/GitManager.h
	src/ImageFinder.cpp
	src/ImageFinder.h
	src/ImageHelper.cpp
	src/ImageHelper.h
	src/KeyboardHook.cpp
	src/KeyboardHook.h
	src/Magnifier.cpp
	src/Magnifier.h
	src/ProcessControl.cpp
	src/ProcessControl.h
	src/ProcessManager.cpp
	src/ProcessManager.h
	src/ProcessUtils.cpp
	src/ProcessUtils.h
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
target_link_libraries(${TARGET} Threads::Threads)

if(UNIX)
	set(MyPrefix "")
	if(APPLE)
		set(MySuffix1 "Mac")
	else(APPLE)
		set(MySuffix1 "Lin")
	endif(APPLE)
	if(TARGET_PLATFORM_32)
		set(MySuffix2 "32")
	else()
		set(MySuffix2 "64")
	endif()
else(UNIX)
	if(NOT MSVC)
		message(FATAL_ERROR "Must be compiled with MSVC on Windows")
	endif(NOT MSVC)
	set(MyPrefix "lib")
	set(MySuffix1 "Win")
endif(UNIX)

set (LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin${MySuffix2})
set (EXECUTABLE_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH})
set (CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})

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

if(UNIX)
	if(TARGET_PLATFORM_32)
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
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
	target_compile_definitions(${TARGET} PRIVATE _WINDOWS
			_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
	target_compile_options(${TARGET} PRIVATE /utf-8)
endif(UNIX)

if(WITH_OPENCV)
	MESSAGE(STATUS "WITH_OPENCV")
	add_definitions(-DUSE_OPENCV)
	if(UNIX)
		find_package(OpenCV REQUIRED PATHS "${CMAKE_SOURCE_DIR}/opencv-x${MySuffix2}" NO_DEFAULT_PATH)
		target_include_directories(${TARGET} PRIVATE ${OpenCV_INCLUDE_DIRS})
		# X11Screenshot.cpp под #ifndef _WINDOWS включает <png.h> и вызывает png_*
		# напрямую; png.h в opencv-x<разрядность> не устанавливается, берём из
		# исходников OpenCV. zlib.h не нужен никому — include на 3rdparty/zlib убран.
		target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/opencv-4.5.2/3rdparty/libpng")
		# libpng и zlib — статические сборки самого OpenCV, а не системные: в
		# OpenCVModules.cmake это IMPORTED-таргеты с путями внутрь opencv-x<разрядность>.
		# Формально они дублируют INTERFACE_LINK_LIBRARIES у opencv_imgcodecs, но
		# оставлены намеренно: X11Screenshot.cpp вызывает png_* сам, а при статической
		# линковке порядок архивов важен (opencv_imgcodecs -> libpng -> zlib).
		target_link_libraries(${TARGET} opencv_core opencv_imgcodecs opencv_imgproc libpng zlib)
	else(UNIX)
		# На Windows find_package(OpenCV) использовать нельзя: OpenCV 4.5.2 не знает
		# MSVC 19.5x (VS 2026), не определяет OpenCV_RUNTIME и раскладывает конфиг по
		# вложенному пути x86/vcNN, которого в нашей плоской установке нет — поиск
		# пакета падает. Поэтому берём библиотеки прямо из префикса установки.
		# Версия в именах .lib (opencv_core452.lib) не хардкодится — ищем по маске.
		set(OCV_DIR "${CMAKE_SOURCE_DIR}/opencv-x${MySuffix2}")
		file(GLOB OCV_IMPORT_LIBS
			"${OCV_DIR}/opencv_core*.lib"
			"${OCV_DIR}/opencv_imgcodecs*.lib"
			"${OCV_DIR}/opencv_imgproc*.lib")
		target_include_directories(${TARGET} PRIVATE "${OCV_DIR}")
		target_link_directories(${TARGET} PRIVATE "${OCV_DIR}")
		target_link_libraries(${TARGET} ${OCV_IMPORT_LIBS} libpng.lib zlib.lib)
	endif(UNIX)
endif(WITH_OPENCV)

if(WITH_BOOST)
	MESSAGE(STATUS "WITH_BOOST")
	add_definitions(-DUSE_BOOST)
	set(Boost_USE_STATIC_LIBS ON) 
	set(Boost_USE_MULTITHREADED ON)  
	set(Boost_USE_STATIC_RUNTIME ON) 
	set(Boost_NO_SYSTEM_PATHS TRUE) 
	set(BOOST_ROOT "${CMAKE_SOURCE_DIR}/boost_1_92_0")
	set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
	set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/stage/lib")
	# FindBoost из CMake < 3.20 знает Boost только до 1.66 и на 1.92 предупреждает
	# про неверные зависимости компонентов. Переменная появилась в CMake 3.20.
	set(Boost_NO_WARN_NEW_VERSIONS ON)
	find_package(Boost REQUIRED regex filesystem)
	include_directories(${Boost_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
endif(WITH_BOOST)

if(NOT UNIX AND WITH_LIBGIT2)
	MESSAGE(STATUS "WITH_LIBGIT2")
	add_definitions(-DUSE_LIBGIT2)
	target_include_directories(${TARGET} PRIVATE libgit2-1.1.0/include)
	set(EMBED_SSH_PATH "${CMAKE_SOURCE_DIR}/libssh2-1.9.0" CACHE PATH "" FORCE)
	add_subdirectory(./libgit2-1.1.0)
	target_link_libraries(${TARGET} git2)
	# libssh2 1.9.0 и сам libgit2 — вендорный код: C4267 (size_t -> uint32_t),
	# C4244 (ssize_t/__int64 -> int в ssh.c и xdiff) и C5287 (сравнение значений
	# разных enum в submodule.c) приходят из их исходников и к нашему коду
	# отношения не имеют. Патчить чужие исходники не будем — глушим эти
	# предупреждения только у целей libgit2, чтобы не потерять их в собственных
	# файлах компоненты.
	if(MSVC)
		foreach(libgit2_target git2internal git2 git2package)
			if(TARGET ${libgit2_target})
				target_compile_options(${libgit2_target} PRIVATE /wd4267 /wd4244 /wd5287)
			endif()
		endforeach()
	endif()
endif(NOT UNIX AND WITH_LIBGIT2)

if(UNIX AND NOT APPLE)
	include (${CMAKE_ROOT}/Modules/FindX11.cmake)
	include_directories(${X11_INCLUDE_DIR})
	target_link_libraries(${TARGET} ${X11_LIBRARIES})

	include (${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules/FindXRandR.cmake)
	include_directories(${XRANDR_INCLUDE_DIR})
	target_link_libraries(${TARGET} ${XRANDR_LIBRARIES})

    set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-Wl,--no-undefined -Xlinker --version-script -Xlinker ${CMAKE_CURRENT_SOURCE_DIR}/src/version.script" )

    SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ON)
    find_program(STRIP NAME eu-strip HINTS ENV PATH DOC "eu-strip executable" )
    if(STRIP)
        SET(STRIP_CMD ${STRIP} $<TARGET_FILE:${PROJECT_NAME}> -f ${LIBRARY_OUTPUT_PATH}/$<TARGET_FILE_NAME:${PROJECT_NAME}>.debug)
		add_custom_command(TARGET ${PROJECT_NAME} 
			POST_BUILD COMMAND ${STRIP_CMD} 
			COMMENT "Strip ${PROJECT_NAME}" 
			MAIN_DEPENDENCY ${PROJECT_NAME}
		)
	else(STRIP)
        message(WARNING "eu-strip not found!")
    endif(STRIP)
endif(UNIX AND NOT APPLE)

if(WITH_GHERKIN)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
	include_directories(${VanessaExt_BINARY_DIR})
	add_dependencies(${TARGET} GherkinLexer)
endif(WITH_GHERKIN)

if(VANESSAEXT_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif(VANESSAEXT_BUILD_TESTS)
