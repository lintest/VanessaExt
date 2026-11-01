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

### Сборка в CI

Основной контур — GitHub Actions, файл `.github/workflows/build.yml`:

* job **linux** собирает `.so` в контейнере `ubuntu:18.04` (скрипт
  `scripts/ci-linux-build.sh`), проверяет ABI-базовую линию и выгружает
  `AddIn.zip` с двумя библиотеками и их `.debug`;
* job **linux-test** (ubuntu-22.04) ставит учебную платформу 1С, поднимает
  Xvfb с оконным менеджером openbox, собирает `.epf` с Linux-библиотекой и
  прогоняет автотесты в тонком клиенте (скрипт `scripts/ci-linux-test.sh`);
* job **windows** ставит платформу 1С, собирает зависимость-зависимые
  библиотеки (boost, OpenCV, libgit2/libssh2, RE-flex), собирает Win32/Win64,
  пакует `AddIn.zip` и `Debug.zip` скриптом `scripts/ci-pack-windows.ps1`,
  собирает `.epf`, прогоняет автотест в тонком клиенте и на `master` создаёт
  черновик релиза.

В job **linux** также прогоняются C++ юнит-тесты (`ProcessUtilsTest` через
ctest, см. раздел ниже).

Платформа для Linux-прогона берётся из ветки `platform-linux` репозитория
`lintest/tools1c`; каталог `~/1c/dist` кэшируется по SHA этой ветки. Образ
`ubuntu-22.04`, а не 24.04: платформа собрана под WebKitGTK 4.0
(`libwebkit2gtk-4.0`, `libsoup-2.4`). Заодно это проверка ABI-базовой линии —
`.so` собрана на 18.04 и должна загрузиться на более новой системе.

Часть автотестов платформозависима, поэтому в
`Example/Forms/Autotests/Ext/Form/Module.bsl` набор фильтруется функцией
`ЭтоЛинукс()`:

* `Тест_УправлениеПриложением`, `Тест_УвеличениеЭкрана`,
  `Тест_МетодыВизуаилзации` — построены на UIAutomation/WinAPI/Magnifier и
  объявлены в `WindowsControl.cpp` внутри `#ifdef _WINDOWS`;
* `Тест_ФункционалGit` и проверка компоненты `GitFor1C` — libgit2
  собирается только на Windows (`CMakeLists.txt`: `NOT UNIX AND WITH_LIBGIT2`);
* проверки «занятого файла» в `Тест_ОшибкиПарсингаGherkin` — в POSIX файл,
  открытый на запись, доступен на чтение, поэтому эти проверки только для
  Windows.

Оконный менеджер в Linux-прогоне обязателен: без WM не работают активное окно,
позиционирование и максимизация, которые проверяют
`Тест_СвойстваУправлениеОкнами` и `Тест_МетодыУправлениеОкнами`.

Версия сборки складывается из файла `VERSION` (база, сейчас `1.3.9`) и номера
прогона: `1.3.9.<run_number>`. Для job'а windows нужен секрет репозитория
`KEY_1CV8T` — пароль к архиву платформы в `lintest/tools1c`.

Автотест запускает внешнюю обработку `Example/Forms/AppVeyor`, а та при открытии
читает `app_port.txt` и отправляет результаты в API AppVeyor. В GitHub Actions
API нет, поэтому на раннере поднимается локальный приёмник
`scripts/ci-appveyor-stub.py`; он же пишет `tests-report.json` и
`tests-report.log` в артефакты. Код 1С при этом не меняется.

AppVeyor (`appveyor.yml`) сохранён как резервный контур: на хостинговом
windows-раннере GitHub нет гарантированной интерактивной сессии, а клиенту 1С
нужно окно. Черновики релизов AppVeyor больше не публикует — это делает только
GitHub Actions.

### Совместимость артефакта с дистрибутивами (ABI-базовая линия)

Собранная библиотека должна загружаться на Ubuntu 18.04 (glibc 2.27,
GLIBCXX_3.4.25) и на Ubuntu 20.04 (glibc 2.31, GLIBCXX_3.4.28). Требования
к версиям символов задаёт не исходный код, а окружение компоновки:

* glibc/libm — версия glibc сборочной машины (`statx` требует 2.28, новый
  `exp/log/pow` — 2.29, `__libc_single_threaded` — 2.32);
* libstdc++ — версия рантайма, найденного при линковке. Например,
  `std::condition_variable::wait()` на машине с libstdc++ из GCC 12
  помечается как `GLIBCXX_3.4.30`, и компонента перестаёт загружаться
  на 18.04/20.04 — 1С показывает диалог установки компоненты при каждом
  запуске, потому что `ПодключитьВнешнююКомпоненту` падает на `dlopen`.

Поэтому релизные Linux-сборки делаются в окружении с нужной базовой линией
(Ubuntu 18.04 + gcc-7, см. выше), а результат проверяется скриптом:

```bash
python3 tools/check_abi_baseline.py --glibc 2.27 --glibcxx 3.4.25 \
    bin64/VanessaExtLin64.so bin32/VanessaExtLin32.so
```

Проверка встроена в CI (job Linux). При необходимости пороги можно
ужесточить или ослабить, например для сборки под более старый дистрибутив:
`--glibc 2.17 --glibcxx 3.4.19`.

Для сборки и запуска тестов ядра процессов:
```bash
cmake -S . -B build \
  -DWITH_GHERKIN=OFF -DWITH_OPENCV=OFF -DWITH_BOOST=OFF \
  -DWITH_LIBGIT2=OFF -DVANESSAEXT_BUILD_TESTS=ON
cmake --build build --target ProcessUtilsTest
ctest --test-dir build --output-on-failure
```
Тестовый таргет `ProcessUtilsTest` собирает только `src/ProcessUtils.cpp`
и не требует X11/OpenCV/Boost. Артефакт теста пишется в `build/tests`,
а не в исходное дерево. Для сборки всей библиотеки используйте `./build.sh`
или обычный `cmake --build build` с установленными зависимостями.

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

Установка современного компилятора С++ на CentOS 7.7
```bash
sudo yum install -y centos-release-scl
sudo yum install -y devtoolset-9-gcc*
scl enable devtoolset-9 bash
```

Установка на VirtualBox дополнений гостевой ОС для CentOS:
```bash
yum group install -y "Development Tools"
yum install -y kernel-devel
mkdir -p /media/cdrom
mount -r /dev/cdrom /media/cdrom
cd /media/cdrom
./VBoxLinuxAdditions.run
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

## Автотесты компоненты без платформы 1С (OneScript)

Часть автотестов компоненты не требует ни платформы 1С, ни GUI: ей достаточно
нативной внешней компоненты. Такой набор гоняется через
[OneScript](https://oscript.io/) — независимую реализацию виртуальной машины
языка 1С.

Запуск из корня репозитория (пути к `keywords.json` и картинкам отсчитываются
от текущего каталога):

```bash
oscript tests/oscript/runner.os --библиотека=bin64/VanessaExtLin64.so
```

Если `--библиотека` не указан, раннер сам ищет собранную библиотеку в
`bin64`/`bin32` (`*Lin64.so` на Linux, `*Win64.dll` на Windows).

Что делает раннер:
1. подключает собранную библиотеку как нативную внешнюю компоненту;
2. загружает существующие модули `Example/Forms/xUnitBDD` и
   `Example/Forms/Autotests`, вырезав директивы `&НаКлиенте` и `&НаСервере` —
   OneScript их не поддерживает;
3. играет роль владельца тестов (те же методы, что у формы `AppVeyor`:
   `Добавить`, `ДобавитьШаг`, `ЗаписатьПроблему`, `ПрерватьТест`);
4. завершается с ненулевым кодом, если хотя бы один тест упал.

Чтобы модули тестов оставались загружаемыми в обе среды, платформенные вызовы
в них спрятаны в `Вычислить` (`СистемнаяИнформация`, `ТипПлатформы`,
`ЧтениеZipФайла`, `Картинка`, `COMОбъект`), а `ЭтоЛинукс()` и `ЭтоКартинка()`
не используют платформенных объектов. При правке тестов это нужно сохранять.

Что заведомо не проверяется в этом контуре: UIAutomation, виртуальные рабочие
столы, увеличение экрана, визуализация, звук и прочий Windows-only функционал —
он остаётся за GUI-прогоном в AppVeyor.

***

При разработке использовались библиотеки:
- [cpp-c11-make-screenshot by Roman Shuvalov](https://github.com/Butataki/cpp-x11-make-screenshot)
- [Clip Library by David Capello](https://github.com/dacap/clip)
- [Boost C++ Libraries](https://www.boost.org/)
