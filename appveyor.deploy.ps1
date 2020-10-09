$account = "lintest"
$project = "VanessaExt"
$name = "VanessaExt"

#https://ci.appveyor.com/api-keys
$token = $env:API_TOKEN 
$url1c = $env:URL_1CV8T

$apiUrl = 'https://ci.appveyor.com/api'
$headers = @{
  "Authorization" = "Bearer $token"
  "Content-type"  = "application/json"
}

# get project with last build details
$project = Invoke-RestMethod -Method Get -Uri "$apiUrl/projects/$account/$project" -Headers $headers

$path = $env:APPVEYOR_BUILD_FOLDER
	
$jobId = $project.build.jobs[0].jobId
$artifacts = Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jobId/artifacts" -Headers $headers
$artifactFileName = $artifacts[0].fileName
Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jobId/artifacts/$artifactFileName" `
  -OutFile "$path\Windows.zip" -Headers @{ "Authorization" = "Bearer $token" }
Expand-Archive -Force -Path "$path\Windows.zip" -DestinationPath $path

$jobId = $project.build.jobs[1].jobId
$artifacts = Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jobId/artifacts" -Headers $headers
$artifactFileName = $artifacts[0].fileName
Invoke-RestMethod -Method Get -Uri "$apiUrl/buildjobs/$jobId/artifacts/$artifactFileName" `
  -OutFile "$path\Linux.zip" -Headers @{ "Authorization" = "Bearer $token" }
Expand-Archive -Force -Path "$path\Linux.zip" -DestinationPath $path

$version = Get-Content -Path "$path\version.txt"
$postfix = '_' + $version -replace '\.', '_'
Update-AppveyorBuild -Version "$version"
Write-Output "Version: $version"

Rename-Item "$path\lib${name}Win32.dll" "${name}Win32$postfix.dll"
Rename-Item "$path\lib${name}Win64.dll" "${name}Win64$postfix.dll"
Rename-Item "$path\lib${name}Lin32.so" "${name}Lin32$postfix.so"
Rename-Item "$path\lib${name}Lin64.so" "${name}Lin64$postfix.so"

$compress = @{
  Path            = "$path\$name*.dll", "$path\$name*.so", "$path\manifest.xml"
  DestinationPath = "$path\AddIn.zip"
}
Compress-Archive @compress

New-Item -ItemType Directory -Force -Path "$path\Example\Templates\$name\" | Out-Null
New-Item -ItemType Directory -Force -Path "$path\Example\Templates\$name\Ext\" | Out-Null
Copy-Item -Path "$path\AddIn.zip" -Destination "$path\Example\Templates\$name\Ext\Template.bin"

$dist1c = "$path\1cv8.zip"
if (!(Test-Path $dist1c)) {
  Invoke-WebRequest -Uri $url1c -OutFile $dist1c
}
Expand-Archive -Force -Path $dist1c -DestinationPath $path

& "bin\1cv8t.exe" DESIGNER /F "$path\Data" /LoadExternalDataProcessorOrReportFromFiles "Example.xml" "$name.epf"  /Out"Log.log"
