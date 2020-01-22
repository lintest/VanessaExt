"%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" SetWindow.sln /property:Configuration=Release /property:Platform=x86
"%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" SetWindow.sln /property:Configuration=Release /property:Platform=x64

oscript .\MakePack.os

copy /b .\AddIn.zip .\Example\Templates\_1cWinCtrl\Ext\Template.bin

oscript .\tools\Compile.os .\