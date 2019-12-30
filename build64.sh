#!/bin/sh

#build script for CentOS 5

PATH=~/bin:$PATH
set -e

export SCL=`which scl`
if [ ! -z $SCL ]; then
    export HAS_DTS=`scl -l | grep devtoolset-2`
fi

cmake -E echo "Build 64" ; 
if [ ! -d build64 ]; then
    cmake -E make_directory build64
    cd build64 ;
    if [ "${SCL}" != "" -a "${HAS_DTS}" != "" ]; then
        scl enable devtoolset-2 'cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF --build .. ';
    else
        cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF --build .. ;
    fi
    cd ..
fi ;
cd build64
cmake --build .
cd ..
#cmake -E remove_directory build64
