#!/bin/sh

PATH=~/bin:$PATH
set -e

SCL=$(command -v scl || true)
export SCL

if [ -n "$SCL" ]; then
  HAS_DTS=$(scl -l | grep devtoolset-2)
  export HAS_DTS
fi

if [ -n "$1" ]; then
  build32=0
  build64=0
  clear=0
  if [ "$1" -eq 32 ]; then
    build32=1
  fi
  if [ "$1" -eq 64 ]; then
    build64=1
  fi
  if [ "$1" = 0 ]; then
    clear=1
  fi
else
  build32=1
  build64=1
  clear=1
fi

if [ $clear -eq 1 ]; then
  cmake -E remove_directory build32L
  cmake -E remove_directory build64L
fi

if [ $build32 -eq 1 ]; then
  cmake -E echo "Build 32"
  if [ ! -d build32L ]; then
    cmake -E make_directory build32L
    cd build32L
    if [ "${SCL}" != "" ] && [ "${HAS_DTS}" != "" ]; then
      scl enable devtoolset-2 'cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=ON --build .. '
    else
      cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=ON --build ..
    fi
    cd ..
  fi
  cd build32L
  cmake --build .
  cd ..
fi    

if [ $build64 -eq 1 ]; then
  cmake -E echo "Build 64"
  if [ ! -d build64L ]; then
    cmake -E make_directory build64L
    cd build64L
    if [ "${SCL}" != "" ] && [ "${HAS_DTS}" != "" ]; then
      scl enable devtoolset-2 'cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF --build .. '
    else
      cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL=OFF --build -dWITH_BOOST:BOOL=ON ..
    fi
    cd ..
  fi ;
  cd build64L
  cmake --build .
  cd ..
fi    

if [ -z "$1" ]; then
  cmake -E remove_directory build32
  cmake -E remove_directory build64
  strip -s bin/libVanessaExt*.so
  rm -f ./*.debug
  rm -f ./*.a

  if [ -n "$(command -v oscript)" ]; then
    Path1C=$(command -v 1cv8 || true)
    export Path1C
    oscript ./tools/ZipLibrary.os
    cp -f ./AddIn.zip ./Example/Templates/VanessaExt/Ext/Template.bin
    oscript ./tools/Compile.os ./
  fi
fi

