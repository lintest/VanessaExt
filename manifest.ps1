Param (
    [string]$project = $env:APPVEYOR_PROJECT_NAME,
    [string]$version = $env:APPVEYOR_BUILD_VERSION
)

$postfix = '_' + $version -replace '\.','-'
$v1,$v2,$v3,$v4 = $version.split('.')
Set-Content 'version.h' "#define VER_FILENAME $project"
Add-Content 'version.h' "#define VERSION_FULL $version"
Add-Content 'version.h' "#define VERSION_MAJOR     $v1"
Add-Content 'version.h' "#define VERSION_MINOR     $v2"
Add-Content 'version.h' "#define VERSION_REVISION  $v3"
Add-Content 'version.h' "#define VERSION_BUILD     $v4"

$encoding = [System.Text.Encoding]::UTF8
$writer = New-Object System.XMl.XmlTextWriter('./manifest.xml', $encoding)
$writer.Formatting = 'Indented'
$writer.Indentation = 1
$writer.IndentChar = "`t"
$writer.WriteStartDocument()
$writer.WriteStartElement('bundle')
$writer.WriteAttributeString('xmlns', 'http://v8.1c.ru/8.2/addin/bundle')

$writer.WriteStartElement('component')
$writer.WriteAttributeString('type', 'native')
$writer.WriteAttributeString('os', 'Windows')
$writer.WriteAttributeString('arch', 'i386')
$writer.WriteAttributeString('path', "${project}Win32${postfix}.dll")
$writer.WriteEndElement();

$writer.WriteStartElement('component')
$writer.WriteAttributeString('type', 'native')
$writer.WriteAttributeString('os', 'Windows')
$writer.WriteAttributeString('arch', 'x86_64')
$writer.WriteAttributeString('path', "${project}Win64${postfix}.dll")
$writer.WriteEndElement();

$writer.WriteStartElement('component')
$writer.WriteAttributeString('type', 'native')
$writer.WriteAttributeString('os', 'Linux')
$writer.WriteAttributeString('arch', 'i386')
$writer.WriteAttributeString('path', "${project}Lin32${postfix}.so")
$writer.WriteEndElement();

$writer.WriteStartElement('component')
$writer.WriteAttributeString('type', 'native')
$writer.WriteAttributeString('os', 'Linux')
$writer.WriteAttributeString('arch', 'x86_64')
$writer.WriteAttributeString('path', "${project}Lin64${postfix}.so")
$writer.WriteEndElement();

$writer.WriteEndElement();
$writer.WriteEndDocument()
$writer.Flush()
$writer.Close()
