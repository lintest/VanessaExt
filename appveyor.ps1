param(
  [string]$account = $env:APPVEYOR_ACCOUNT_NAME,
  [string]$project = $env:APPVEYOR_PROJECT_NAME,
  [string]$name = $env:APPVEYOR_PROJECT_NAME
)

$path = $env:APPVEYOR_BUILD_FOLDER
$version = $env:APPVEYOR_BUILD_VERSION
$postfix = '_' + $version -replace '\.', '-'

$apiUrl = 'https://ci.appveyor.com/api'
$data = Invoke-RestMethod -Method Get -Uri "$apiUrl/projects/$account/$project/build/$version"
$jobId = $data.build.jobs[0].jobId

Invoke-RestMethod -Method Get -OutFile "$path\Linux.zip" `
  -Uri "$apiUrl/buildjobs/$jobId/artifacts/AddIn.zip" 

Expand-Archive -Force -Path "$path\Linux.zip" -DestinationPath $path
Rename-Item "$path\${name}Win32.dll" "${name}Win32$postfix.dll"
Rename-Item "$path\${name}Win64.dll" "${name}Win64$postfix.dll"
Rename-Item "$path\${name}Lin32.so" "${name}Lin32$postfix.so"
Rename-Item "$path\${name}Lin64.so" "${name}Lin64$postfix.so"

$compress = @{
  Path            = "$path\$name*.dll", "$path\$name*.so", "$path\manifest.xml"
  DestinationPath = "$path\AddIn.zip"
}
Compress-Archive @compress

New-Item -ItemType Directory -Force -Path "$path\Example\Templates\$name\Ext\" | Out-Null
Copy-Item -Path "$path\AddIn.zip" -Destination "$path\Example\Templates\$name\Ext\Template.bin"
Set-Content -Path "$path\app_port.txt" -Value ([uri] $env:APPVEYOR_API_URL).Port
