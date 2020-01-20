#!/bin/sh

PATH=~/bin:$PATH
set -e

export SCL=`which scl`
if [ ! -z $SCL ]; then
    export HAS_DTS=`scl -l | grep devtoolset-2`
fi

if [ -n "$1" ]; then
    if [ $1 -eq 32 ]; then
        build32=1
        build64=0
    fi
    if [ $1 -eq 64 ]; then
        build32=0
        build64=1
    fi
else
    build32=1
    build64=1
fi

if [ $build32 -eq 1 ]; then
    cmake -E echo "Build 32" 
    if [ ! -d build32 ]; then
        cmake -E make_directory build32
        cd build32
        if [ "${SCL}" != "" -a "${HAS_DTS}" != "" ]; then
            scl enable devtoolset-2 'cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=ON --build .. '
        else
            cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=ON --build ..
        fi
        cd .. 
    fi 
    cd build32 
    cmake --build .
    cd ..
fi    

if [ $build64 -eq 1 ]; then
    cmake -E echo "Build 64"
    if [ ! -d build64 ]; then
        cmake -E make_directory build64
        cd build64
        if [ "${SCL}" != "" -a "${HAS_DTS}" != "" ]; then
            scl enable devtoolset-2 'cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF --build .. '
        else
            cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF --build ..
        fi
        cd ..
    fi ;
    cd build64
    cmake --build .
    cd ..
fi    

# cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-x86_64-w64-mingw32.cmake -D WINDOWS:BOOL=ON --build ..
# cmake --build .

if [ -z $1 ]; then
    cmake -E remove_directory build32
    cmake -E remove_directory build64
    strip -s bin/*
    rm -f *.{debug,a}
fi

cd ..
