# Cmake script for project AddIn
# autor: 1C
cmake_minimum_required (VERSION 3.1)

if (WIN32)
    if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio ([0-9]+)")
        if (${CMAKE_MATCH_1} GREATER_EQUAL 12) 
            IF (NOT "${CMAKE_GENERATOR_TOOLSET}" STREQUAL "v120_xp")
                set(CMAKE_GENERATOR_TOOLSET "v120_xp" )
                set(CMAKE_VS_PLATFORM_TOOLSET "v120_xp" )
            ENDIF()
        else()
            message(FATAL_ERROR "Supported Visual Studio: 12 (v120_xp), 14 (v120_xp), 15 (v120_xp) or above with v120_xp toolset")
        endif()

    endif()
    IF(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.15.0)
        cmake_policy(SET CMP0091 NEW)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"  CACHE STRING "Runtime lib" FORCE)
    endif()
endif(WIN32)
SET(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "Build configurations" FORCE)
SET(CMAKE_BUILD_TYPE Release CACHE STRING "Build configurations" FORCE)

project (AddIn)
#-----------------------------------------------------------------------------
# Minimum compiler version check: Microsoft C/C++ >= 18.0 (aka VS 2013 aka VS 12.0)
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 18.0)
  message(FATAL_ERROR "Only Microsoft Visual Studio 2013 or later is currently supported.")
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    IF(${CMAKE_VERSION} VERSION_LESS 3.15.0)
        foreach(flag
           CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
           CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
                set(${flag} ${${flag}} CACHE "STRING" "Compilation flags" FORCE)
            endif()
        endforeach()
    endif()
endif()
if (WIN32)
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 8 OR "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "x64")
        OPTION(TARGET_PLATFORM_32 "Build target for i386" OFF)
    else()
        OPTION(TARGET_PLATFORM_32 "Build target for i386" ON)
    endif()
else()
    OPTION(TARGET_PLATFORM_32 "Build target for i386" ON)
endif()

MARK_AS_ADVANCED(TARGET_PLATFORM_32)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#-----------------------------------------------------------------------------
# Minimum compiler version check: GCC >= 4.8
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
  message(FATAL_ERROR "GCC 4.8 or later is required.")
endif ()

if (NOT UNIX)
    IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
       SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Compilator type" FORCE)
    ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)   
endif(NOT UNIX)

if (UNIX)
    set(MySuffix1 "Lin")
    if (APPLE)
        set(MySuffix1 "Mac")
    endif(APPLE)
else(UNIX)
    set(MySuffix1 "Win")
endif(UNIX)

set(BUILD_DIR "build")
set(CMAKE_SKIP_RPATH ON CACHE "BOOL" "" FORCE)

if (TARGET_PLATFORM_32)
    set(MySuffix2 "32")
    if (UNIX)
        SET(ARCH_FLAG "-m32")
    endif()
else()
    set(MySuffix2 "64")
    if (UNIX)
        SET(ARCH_FLAG "-m64")
    endif()
endif()

if (UNIX)
    foreach(flag
        CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_C_FLAGS CMAKE_CXX_FLAGS )
        if(NOT ${flag} MATCHES ${ARCH_FLAG})
            set(${flag} ${${flag}} ${ARCH_FLAG} CACHE "STRING" "Linker flags" FORCE)
        endif()
    endforeach()
endif()

set (LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set (EXECUTABLE_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH})
set (CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})

IF(WIN32 AND MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
ENDIF()

# identification AddIn
SET(MYCOMPANY "Lintest")
SET(ADN_NAME "VanessaExt")

#set you version file
IF(EXISTS "${CMAKE_SOURCE_DIR}/../lib/VerInfo.txt")
    FILE(STRINGS "${CMAKE_SOURCE_DIR}/../lib/VerInfo.txt" VERSION LIMIT_COUNT 1)
ELSE()
    SET(VERSION 001)
ENDIF()

include_directories(BEFORE  . ${CMAKE_SOURCE_DIR}/include)

add_subdirectory (src)

