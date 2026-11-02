param([string]$JobId, [string]$Out)
$t = $env:GH_TOKEN
$h = @{ Authorization = "Bearer $t"; 'User-Agent' = 'ci'; Accept = 'application/vnd.github+json' }
try {
    $r = Invoke-WebRequest -Uri "https://api.github.com/repos/lintest/VanessaExt/actions/jobs/$JobId/logs" -Headers $h -OutFile $Out -PassThru
    Write-Host ("status=" + $r.StatusCode + " size=" + (Get-Item $Out).Length)
} catch {
    Write-Host ("ERR " + $_.Exception.Message)
}
