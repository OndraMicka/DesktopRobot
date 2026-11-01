param([string]$Compiler = '')

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$buildDir = Join-Path $repoRoot 'testing/build'
$program = Join-Path $buildDir 'desktop_robot_simulator.exe'

if (-not $Compiler) {
    $found = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($found) { $Compiler = $found.Source }
    elseif (Test-Path -LiteralPath 'C:/msys64/ucrt64/bin/g++.exe') {
        $Compiler = 'C:/msys64/ucrt64/bin/g++.exe'
    } else { throw 'g++.exe not found. Pass -Compiler with the MSYS2 UCRT64 compiler path.' }
}
$compilerPath = (Get-Command $Compiler -ErrorAction Stop).Source
$compilerDir = Split-Path $compilerPath -Parent
$toolchainRoot = Split-Path $compilerDir -Parent

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
# Compile every simulator implementation, plus the firmware modules shared by
# the simulator. VS Code invokes this same script instead of keeping a third list.
$sources = @(Get-ChildItem (Join-Path $PSScriptRoot 'src') -Recurse -Filter '*.cpp' |
    Sort-Object FullName | ForEach-Object { $_.FullName })
$sources += @(
    'esp32/src/display/animations/CharacterAnimations.cpp'
    'esp32/src/behavior/BehaviourProgram.cpp'
    'esp32/src/display/animations/TouchWakeAnimation.cpp'
    'esp32/src/display/animations/TouchLookUpAnimation.cpp'
    'esp32/src/sound/SoundData.cpp'
) | ForEach-Object { Join-Path $repoRoot $_ }
$compilerArgs = @(
    '-std=c++17', '-g', '-Wall', '-Wextra',
    "-I$(Join-Path $toolchainRoot 'include')",
    "-L$(Join-Path $toolchainRoot 'lib')",
    "-I$(Join-Path $PSScriptRoot 'src')"
) + $sources + @('-lSDL2', '-o', $program)

$originalPath = $env:PATH
try {
    # GCC's compiler/linker subprocesses need the toolchain DLLs as well.
    $env:PATH = "$compilerDir;$originalPath"
    & $compilerPath @compilerArgs
    $buildExitCode = $LASTEXITCODE
} finally { $env:PATH = $originalPath }
if ($buildExitCode -ne 0) {
    throw 'Simulator build failed. Close any running simulator before rebuilding; see compiler errors above.'
}

# Also allow launching the executable directly, without a VS Code-specific PATH.
foreach ($name in @('SDL2.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')) {
    $runtime = Join-Path $compilerDir $name
    if (Test-Path -LiteralPath $runtime) {
        Copy-Item -LiteralPath $runtime -Destination (Join-Path $buildDir $name) -Force
    }
}
Write-Output "Built simulator: $program"
