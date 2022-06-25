rem SET libgit2v="1.4.3"
rem SET libssh2v="1.9.0"

rem if NOT EXIST "%CD%\libgit2-%libgit2v%" bitsadmin /transfer mydownloadjob /download /priority FOREGROUND "https://github.com/libgit2/libgit2/archive/v%libgit2v%.zip" "%CD%\libgit2-%libgit2v%.zip"
rem if NOT EXIST "%CD%\libgit2-%libgit2v%" powershell Expand-Archive "%CD%\libgit2-%libgit2v%.zip" -DestinationPath "%CD%"

rem if NOT EXIST "%CD%\libssh2-%libssh2v% " bitsadmin /transfer mydownloadjob /download /priority FOREGROUND "https://github.com/libssh2/libssh2/archive/libssh2-%libssh2v%.zip" "%CD%\libssh2-%libssh2v%.zip"
rem if NOT EXIST "%CD%\libssh2-%libssh2v%" powershell Expand-Archive "%CD%\libssh2-%libssh2v%.zip" -DestinationPath "%CD%"
rem ren libssh2-libssh2-1.9.0 libssh2-1.9.0

cmake -E remove_directory build32W
cmake -E remove_directory build64W
del bin\Release\VanessaExtWin32.dll
del bin\Release\VanessaExtWin64.dll

mkdir build32W
cd build32W
cmake .. -A Win32 -DMySuffix2=32
cmake --build . --config Release
cd ..

mkdir build64W
cd build64W
cmake .. -A x64 -DMySuffix2=64
cmake --build . --config Release
cd ..

oscript .\tools\ZipLibrary.os

copy /b .\AddIn.zip .\Example\Templates\VanessaExt\Ext\Template.bin

oscript .\tools\Compile.os .\