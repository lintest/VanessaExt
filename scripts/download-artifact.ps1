param(
  [Parameter(Mandatory = $true)][string]$JobName,
  [Parameter(Mandatory = $true)][string]$ArtifactName,
  [string]$Destination = $ArtifactName
)

$ErrorActionPreference = 'Stop'

$api = 'https://ci.appveyor.com/api'
$build = Invoke-RestMethod "$api/projects/$env:APPVEYOR_ACCOUNT_NAME/$env:APPVEYOR_PROJECT_NAME/build/$env:APPVEYOR_BUILD_VERSION"
$job = $build.build.jobs | Where-Object { $_.name -eq $JobName }

if (-not $job) {
  throw "AppVeyor job '$JobName' not found in build $env:APPVEYOR_BUILD_VERSION"
}

Write-Host "Downloading $ArtifactName from job $JobName ($($job.jobId))"
Invoke-RestMethod -OutFile $Destination "$api/buildjobs/$($job.jobId)/artifacts/$ArtifactName"
