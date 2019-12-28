#!/bin/sh

#build script for CentOS 5

PATH=~/bin:$PATH
set -e

export SCL=`which scl`
if [ ! -z $SCL ]; then
    export HAS_DTS=`scl -l | grep devtoolset-2`
fi
cmake -E echo "Build 32" ; 
if [ ! -d build32 ]; then
    cmake -E make_directory build32
    pushd build32
    if [ "${SCL}" != "" -a "${HAS_DTS}" != "" ]; then
        scl enable devtoolset-2 'cmake -D TARGET_PLATFORM_32:BOOL=ON  -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo --build .. ';
    else
        cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=ON --build .. ;
    fi
    popd
fi ;
pushd build32
cmake --build .
popd 
cmake -E remove_directory build32

cmake -E echo "Build 64" ; 
if [ ! -d build64 ]; then
    cmake -E make_directory build64
    pushd build64 ;
    if [ "${SCL}" != "" -a "${HAS_DTS}" != "" ]; then
        scl enable devtoolset-2 'cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF --build .. ';
    else
        cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF --build .. ;
    fi
    popd
fi ;
pushd build64
cmake --build .
popd
cmake -E remove_directory build64

#strip -s bin/*
pushd bin
rm -f *.{debug,a}
for i in *; do eu-strip -f $i.debug $i ; done
popd

