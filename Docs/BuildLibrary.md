## Самостоятельная сборка библиотеки

Порядок самостоятельной сборки библиотеки внешних компонент из исходников:
1. Для сборки необходимо установить Visual Studio Community 2019
2. Скачиваем и устанавливаем библиотеку [**boost**](http://www.boost.org/users/download/)
3. Чтобы работала сборка примера обработки EPF надо установить OneScript версии 1.0.20 или выше
4. Устанавливаем VirtualBox и разворачиваем в минимальной конфигурации Ubuntu 18.04 или CentOS 8
5. Устанавливаем на Linux необходимые пакеты (см. ниже) и дополнения гостевой ОС
6. Подключаем в VirtualBox общую папку с исходными текстами внешней компоненты
7. В среде Linux для компиляции библиотек запустить ./build.sh
8. В среде Window для завершения сборки запустить ./compile.bat

Сборка для Linux в CentOS 8:
```bash
yum -y group install "Development Tools"
yum -y install cmake glibc-devel.i686 glibc-devel libuuid-devel
yum -y install libstdc++-devel.i686 gtk2-devel.i686 glib2-devel.i686
yum -y install libstdc++-devel.x86_64 gtk2-devel.x86_64 glib2-devel.x86_64
yum -y install libXtst-devel.i686 libXtst-devel.x86_64
git clone https://github.com/lintest/VanessaExt.git
cd VanessaExt
./build.sh
```

Сборка для Linux в Ubuntu 18.04:
```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install -y build-essential cmake git
sudo apt install -y gcc-multilib g++-multilib
sudo apt install -y uuid-dev libx11-dev libxrandr-dev libpng-dev
sudo apt install -y libxtst-dev libxtst-dev:i386
git clone https://github.com/lintest/VanessaExt.git
cd VanessaExt
./build.sh
```

Сборка библотеки [**boost**](https://www.boost.org/doc/libs/1_72_0/more/getting_started/windows.html#prepare-to-use-a-boost-library-binary) для Windows
```Batchfile
b2.exe toolset=msvc link=static threading=multi runtime-link=static release stage
```

Сборка библотеки [**boost**] для Linux
```Batchfile
./b2 cxxflags=-fPIC link=static threading=multi runtime-link=static release stage
```

Установка на VirtualBox дополнений гостевой ОС для Linux:
```bash
mkdir -p /media/cdrom
mount -r /dev/cdrom /media/cdrom
cd /media/cdrom
./VBoxLinuxAdditions.run
sudo usermod -a -G vboxsf "$USER"
reboot
```

Чтобы работала сборка демонстрационной внешней обработки под Linux:
+ Установить [onescript](https://oscript.io/);
+ Добавить в PATH путь к актуальной версии платформы 1с.
  Например, так:
  ```bash
  sudo ln -s /opt/1cv8/x86_64/8.3.18.891/1cv8 /usr/local/bin/1cv8
  ```
  
Для отладки компоненты под Linux:
+ Закомментировать строку _strip -s bin/libVanessaExt*.so_ в файле **build.sh**
+ В файле **CMakeLists.txt** заменить строку:

    _SET(CMAKE_BUILD_TYPE Release CACHE STRING "Build configurations" FORCE)_

  на:
  
    _SET(CMAKE_BUILD_TYPE **Debug** CACHE STRING "Build configurations" FORCE)_
+ При необходимости удалить установленную ранее библиотеку в папке **~/.1cv8/1C/1cv8/ExtCompT/**

Если в момент присоединения к процессу 1с возникает ошибка доступа,
можно выполнить следующую [инструкцию](https://askubuntu.com/questions/41629/after-upgrade-gdb-wont-attach-to-process).

***

При разработке использовались библиотеки:
- [cpp-c11-make-screenshot by Roman Shuvalov](https://github.com/Butataki/cpp-x11-make-screenshot)
- [Clip Library by David Capello](https://github.com/dacap/clip)
- [Boost C++ Libraries](https://www.boost.org/)
