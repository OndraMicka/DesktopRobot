$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '../..')
try {
    New-Item -ItemType Directory -Force esp32/.pio/host | Out-Null
    $sources = @(Get-ChildItem esp32/src -Recurse -Filter *.cpp | Where-Object { $_.Name -ne 'main.cpp' } | ForEach-Object { $_.FullName })
    & g++ -std=c++17 -Wall -Wextra -Werror -Iesp32/host/stubs -Iesp32/src esp32/host/tests.cpp @sources -o esp32/.pio/host/tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Host test compilation failed' }
    & ./esp32/.pio/host/tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Host tests failed' }
} finally { Pop-Location }
