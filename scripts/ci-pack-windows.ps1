# Упаковка артефактов VanessaExt в GitHub Actions.
#
# Повторяет то, что делает appveyor.ps1, но без обращения к AppVeyor API:
# Linux-библиотеки приходят не через api/buildjobs, а как артефакт job'а linux
# (actions/download-artifact кладёт AddIn.zip в каталог linux-addin).
#
# Что делает:
#   1. забирает собранные Win32/Win64 dll из bin32/bin64;
#   2. распаковывает Linux-артефакт и переименовывает все четыре библиотеки
#      (плюс .debug) с постфиксом версии — так их ждёт manifest.xml;
#   3. собирает AddIn.zip (4 библиотеки + manifest.xml) и Debug.zip (.debug);
#   4. укладывает AddIn.zip в макет внешней обработки Example/Templates/VanessaExt,
#      а keywords.json — в макет Keywords, чтобы .epf собирался с компонентой внутри.
#
# Запуск (из корня репозитория, после manifest.ps1 и сборки):
#   powershell -File scripts/ci-pack-windows.ps1 -version 1.3.9.42

param(
	[string]$project = 'VanessaExt',
	[string]$version = $env:BUILD_VERSION,
	[string]$path = (Get-Location).Path,
	[string]$linuxArtifactDir = 'linux-addin'
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($version)) {
	throw 'Не задана версия: передайте -version или переменную окружения BUILD_VERSION'
}

$postfix = '_' + ($version -replace '\.', '-')

function Assert-File($filePath, $what) {
	if (-not (Test-Path $filePath)) {
		throw "Не найден $what : $filePath"
	}
}

# 1. Windows-библиотеки лежат в подкаталогах Release, а упаковщик ждёт их в корне.
foreach ($arch in @('32', '64')) {
	$source = Join-Path $path "bin$arch\Release\${project}Win$arch.dll"
	Assert-File $source "собранный ${project}Win$arch.dll"
	Copy-Item -Path $source -Destination $path -Force
}

# 2. Linux-артефакт. download-artifact с одним файлом кладёт его прямо в
#    linux-addin\AddIn.zip, но структуру не гарантируем — ищем рекурсивно.
$linuxZip = Get-ChildItem -Path (Join-Path $path $linuxArtifactDir) -Filter 'AddIn.zip' -Recurse -ErrorAction SilentlyContinue |
	Select-Object -First 1
if (-not $linuxZip) {
	throw "Не найден Linux-артефакт AddIn.zip в каталоге $linuxArtifactDir"
}
Write-Host "Linux-артефакт: $($linuxZip.FullName)"
Expand-Archive -Force -Path $linuxZip.FullName -DestinationPath $path

$binaries = @(
	"${project}Win32.dll",
	"${project}Win64.dll",
	"${project}Lin32${postfix}.so",
	"${project}Lin64${postfix}.so"
)
$debugFiles = @(
	"${project}Lin32${postfix}.so.debug",
	"${project}Lin64${postfix}.so.debug"
)

# Linux-библиотеки приходят из Linux-артефакта уже с суффиксом версии — так их
# ждёт manifest.xml. Windows-библиотеки приходят без суффикса: добавляем его
# перед расширением (VanessaExtWin32_1-3-9-25.dll), иначе платформа не находит
# файл по пути из манифеста и подключение компоненты молча проваливается.
foreach ($name in @("${project}Win32.dll", "${project}Win64.dll")) {
	$source = Join-Path $path $name
	Assert-File $source "собранный $name"
	$versioned = [System.IO.Path]::GetFileNameWithoutExtension($name) + $postfix +
		[System.IO.Path]::GetExtension($name)
	Rename-Item -Path $source -NewName $versioned -Force
}

$binaries = @(
	"${project}Win32${postfix}.dll",
	"${project}Win64${postfix}.dll",
	"${project}Lin32${postfix}.so",
	"${project}Lin64${postfix}.so"
)

foreach ($name in $binaries) {
	Assert-File (Join-Path $path $name) "файл для AddIn.zip"
}

# .debug появляются только если в контейнере был eu-strip (пакет elfutils),
# поэтому Debug.zip собираем по факту наличия файлов.
foreach ($name in $debugFiles) {
	if (-not (Test-Path (Join-Path $path $name))) {
		Write-Host "пропускаю (нет файла): $name"
	}
}

Assert-File (Join-Path $path 'manifest.xml') 'manifest.xml (его пишет manifest.ps1)'

# 3. Итоговые архивы.
$addInZip = Join-Path $path 'AddIn.zip'
if (Test-Path $addInZip) { Remove-Item $addInZip -Force }

$packed = @()
foreach ($name in $binaries) {
	$packed += (Join-Path $path $name)
}
$packed += (Join-Path $path 'manifest.xml')
Compress-Archive -Path $packed -DestinationPath $addInZip -Force

$debugPacked = @()
foreach ($name in $debugFiles) {
	$file = Join-Path $path $name
	if (Test-Path $file) { $debugPacked += $file }
}
if ($debugPacked.Count -gt 0) {
	Compress-Archive -Path $debugPacked -DestinationPath (Join-Path $path 'Debug.zip') -Force
}
else {
	Write-Host 'пропускаю Debug.zip: файлов .debug нет'
}

# 4. Макеты внешней обработки: компонента и словарь ключевых слов едут внутрь .epf.
$templateDir = Join-Path $path "Example\Templates\$project\Ext"
New-Item -ItemType Directory -Force -Path $templateDir | Out-Null
Copy-Item -Path $addInZip -Destination (Join-Path $templateDir 'Template.bin') -Force

$keywordsDir = Join-Path $path 'Example\Templates\Keywords\Ext'
New-Item -ItemType Directory -Force -Path $keywordsDir | Out-Null
$keywordsZip = Join-Path $path 'keywords.zip'
if (Test-Path $keywordsZip) { Remove-Item $keywordsZip -Force }
Compress-Archive -Path (Join-Path $path 'keywords.json') -DestinationPath $keywordsZip -Force
Copy-Item -Path $keywordsZip -Destination (Join-Path $keywordsDir 'Template.bin') -Force

Write-Host '== Содержимое AddIn.zip'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead($addInZip)
try {
	foreach ($entry in $archive.Entries) {
		Write-Host ("   {0} ({1} байт)" -f $entry.FullName, $entry.Length)
	}
}
finally {
	$archive.Dispose()
}
