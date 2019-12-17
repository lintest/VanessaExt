"%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" SetWindow.sln /property:Configuration=Release /property:Platform=x86
"%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" SetWindow.sln /property:Configuration=Release /property:Platform=x64

powershell Compress-Archive -LiteralPath .\src\MANIFEST.XML -DestinationPath ./AddIn.zip -Update
powershell Compress-Archive -LiteralPath .\bin\WindowsControlWin32.dll -DestinationPath ./AddIn.zip -Update
powershell Compress-Archive -LiteralPath .\bin64\WindowsControlWin64.dll -DestinationPath ./AddIn.zip -Update
copy /b .\AddIn.zip .\Example\Templates\SetWindow\Ext\Template.bin

oscript .\tools\Compile.os .\