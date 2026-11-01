#!/usr/bin/env bash
#
# Сборка Linux-компоненты VanessaExt в контейнере Ubuntu 18.04.
#
# Зачем: версии символов glibc/libstdc++ в .so задаёт сборочное окружение, а не
# исходный код. Образ Ubuntu1804 в AppVeyor давно подменён на 20.04, и артефакт
# начинает требовать GLIBC_2.28/2.29 (statx, exp/log/pow), после чего компонента
# не загружается на Ubuntu 18.04 (glibc 2.27) и 20.04 (2.31).
#
# Скрипт рассчитан на запуск из корня репозитория, смонтированного в контейнер:
#
#   docker run --rm -v "$PWD:/src" -w /src ubuntu:18.04 bash scripts/ci-linux-build.sh
#
# Номер сборки для version.h берётся из BUILD_VERSION (GitHub Actions) или из
# APPVEYOR_BUILD_VERSION (AppVeyor) — скрипт общий для двух CI.
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

echo "== Окружение"
cat /etc/os-release | head -n2
gcc --version 2>/dev/null || true

echo "== Зависимости"
dpkg --add-architecture i386
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
  build-essential cmake git curl ca-certificates \
  gcc-7 g++-7 g++-7-multilib gcc-7-multilib \
  uuid-dev uuid-dev:i386 \
  libpcre3-dev:i386 \
  libx11-dev libx11-dev:i386 \
  libxtst-dev libxtst-dev:i386 \
  libxrandr-dev libxrandr-dev:i386 \
  libpng-dev libpng-dev:i386 \
  libssl-dev libssl-dev:i386 \
  zlib1g-dev elfutils

update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 100 \
  --slave /usr/bin/g++ g++ /usr/bin/g++-7
update-alternatives --set gcc "/usr/bin/gcc-7"
gcc --version

echo "== CMake"
# Системный CMake в Ubuntu 18.04 — 3.10. Его FindBoost знает Boost только до 1.66
# и на 1.92 предупреждает про неверные зависимости компонентов; подавить это
# умеет только Boost_NO_WARN_NEW_VERSIONS из CMake 3.20. Бинарник Kitware собран
# на glibc 2.17, поэтому на 18.04 запускается. Если не запустился — молча
# остаёмся на системном: на результат сборки версия CMake не влияет.
CMAKE_VERSION=3.20.6
if [ ! -x /opt/cmake/bin/cmake ]; then
  if curl -sSfL -o /tmp/cmake.tar.gz \
      "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz"; then
    mkdir -p /opt/cmake
    tar xzf /tmp/cmake.tar.gz -C /opt/cmake --strip-components=1
    rm -f /tmp/cmake.tar.gz
  else
    echo "не удалось скачать CMake ${CMAKE_VERSION} — остаюсь на системном"
  fi
fi
if /opt/cmake/bin/cmake --version >/dev/null 2>&1; then
  export PATH="/opt/cmake/bin:${PATH}"
fi
cmake --version | head -n1

# Контейнер работает от root, а /src смонтирован от uid хоста: без этого git
# отказывается работать с репозиторием и падает с «dubious ownership».
git config --global --add safe.directory '*'

echo "== RE-flex"
# RE-flex в репозитории — gitlink (подмодуль): после checkout это пустая папка,
# поэтому проверяем именно наличие .git, а не каталога.
# Готовый bin/reflex приходит из кэша CI — тогда клон и сборка не нужны.
if [ ! -x RE-flex/bin/reflex ]; then
  if [ ! -d RE-flex/.git ]; then
    rm -rf RE-flex
    git clone -q --branch=master --depth=1 https://github.com/lintest/RE-flex.git RE-flex
  fi
  (
    cd RE-flex
    git checkout -q -B version v3.0.1
    ./clean.sh
    ./build.sh
  )
else
  echo "RE-flex/bin/reflex уже собран — пропускаю"
fi

echo "== OpenCV"
# Build.OpenCV.sh ожидает уже развёрнутый исходник (в старом пайплайне его
# клонировали отдельным шагом install).
# Установленные деревья приходят из кэша CI — тогда сборка пропускается.
if [ ! -d opencv-x32 ] || [ ! -d opencv-x64 ]; then
  if [ ! -d opencv-4.5.2 ]; then
    git clone -q --branch=4.5.2 --depth=1 https://github.com/opencv/opencv.git opencv-4.5.2
  fi
  ./Build.OpenCV.sh
else
  echo "opencv-x32 и opencv-x64 уже собраны — пропускаю"
fi

echo "== Boost"
# Каталог boost_1_92_0 может прийти из кэша CI частично: там лежат только
# заголовки (boost/) и собранные библиотеки (stage/lib), без bootstrap.sh и b2.
# Поэтому проверяем наличие самого скрипта загрузки, а не каталога: tar
# распакуется поверх и добавит недостающие файлы, не тронув stage/lib.
if [ ! -f boost_1_92_0/bootstrap.sh ]; then
  curl -sSfL -o boost_1_92_0.tar.gz \
    https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.gz
  tar xzf boost_1_92_0.tar.gz
fi
# В tar-архиве boost нет готового b2 — его собирает bootstrap.sh.
if [ ! -x boost_1_92_0/b2 ]; then
  ( cd boost_1_92_0 && ./bootstrap.sh )
fi

echo "== version.h"
BUILD_VERSION="${BUILD_VERSION:-${APPVEYOR_BUILD_VERSION:-0.0.0}}"
echo "сборка ${BUILD_VERSION}"
echo "#define VERSION_FULL \"${BUILD_VERSION}\"" > version.h

build() {
  local address_model="$1"
  local build_dir="$2"
  local target_32="$3"

  echo "== Boost ${address_model}-bit"
  (
    cd boost_1_92_0
    ./b2 cxxflags="-fPIC -std=c++17" link=static threading=multi \
      runtime-link=static release stage \
      --with-regex --with-filesystem \
      "address-model=${address_model}"
  )

  echo "== CMake ${address_model}-bit"
  rm -rf "$build_dir"
  mkdir -p "$build_dir"
  (
    cd "$build_dir"
    cmake -D CMAKE_BUILD_TYPE:STRING=RelWithDebInfo -D TARGET_PLATFORM_32:BOOL="${target_32}" ..
    cmake --build . --config RelWithDebInfo
  )
}

build 32 build32 ON
build 64 build64 OFF

echo "== Готово"
ls -la bin32 bin64 2>/dev/null || true
