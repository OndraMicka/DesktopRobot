$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '../..')
try {
    New-Item -ItemType Directory -Force esp32/.pio/host | Out-Null
    $sources = @(Get-ChildItem esp32/src -Recurse -Filter *.cpp | Where-Object { $_.Name -ne 'main.cpp' } | ForEach-Object { $_.FullName })
    & g++ -std=c++17 -DROBOT_COMPACT_ANIMATIONS -Wall -Wextra -Werror -Iesp32/host/stubs -Iesp32/src esp32/host/tests.cpp @sources -o esp32/.pio/host/tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Host test compilation failed' }
    & ./esp32/.pio/host/tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Host tests failed' }
    $graphics = @('testing/simulator/src/Framebuffer.cpp', 'testing/simulator/src/animation/Animation.cpp', 'testing/simulator/src/animation/TestAnimations.cpp')
    & g++ -std=c++17 -Wall -Wextra -Werror -Itesting/simulator/src testing/simulator/tests/StateTransitions.cpp testing/simulator/src/animation/AnimationPlayer.cpp @graphics -o esp32/.pio/host/transitions.exe
    if ($LASTEXITCODE -ne 0) { throw 'Transition test compilation failed' }
    & ./esp32/.pio/host/transitions.exe
    if ($LASTEXITCODE -ne 0) { throw 'Transition tests failed' }
    & g++ -std=c++17 -Itesting/simulator/src testing/simulator/tests/CatalogSnapshot.cpp @graphics -o esp32/.pio/host/snapshot.exe
    if ($LASTEXITCODE -ne 0) { throw 'Catalog snapshot compilation failed' }
    $firmwareGraphics = @(Get-ChildItem esp32/src/display/animations -Filter *.cpp | ForEach-Object { $_.FullName })
    & g++ -std=c++17 -DROBOT_COMPACT_ANIMATIONS -Itesting/simulator/src testing/simulator/tests/CatalogSnapshot.cpp esp32/src/display/Framebuffer.cpp esp32/src/display/Animation.cpp @firmwareGraphics -o esp32/.pio/host/snapshot-compact.exe
    if ($LASTEXITCODE -ne 0) { throw 'Compact catalog snapshot compilation failed' }
    $normalSnapshot = & ./esp32/.pio/host/snapshot.exe
    if ($LASTEXITCODE -ne 0) { throw 'Catalog snapshot failed' }
    $compactSnapshot = & ./esp32/.pio/host/snapshot-compact.exe
    if ($LASTEXITCODE -ne 0 -or $normalSnapshot -ne $compactSnapshot) { throw 'Compact catalog differs from PC catalog' }
    Write-Output "PASS: identical PC/ESP32 animation pixels and durations ($normalSnapshot)"
} finally { Pop-Location }
