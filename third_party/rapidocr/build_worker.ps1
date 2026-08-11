$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$venvPython = Join-Path $root "builder-venv\Scripts\python.exe"
$workerScript = Join-Path $root "worker\rapidocr_worker.py"
$distRoot = Join-Path $root "runtime"
$buildRoot = Join-Path $root "pyinstaller-build"
$specRoot = Join-Path $root "pyinstaller-spec"

if (-not (Test-Path $venvPython)) {
    throw "Missing builder Python at $venvPython"
}

Remove-Item -Recurse -Force $distRoot -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $buildRoot -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force $specRoot -ErrorAction SilentlyContinue

& $venvPython -m PyInstaller `
    --noconfirm `
    --clean `
    --name rapidocr_worker `
    --distpath $distRoot `
    --workpath $buildRoot `
    --specpath $specRoot `
    --collect-all rapidocr `
    --collect-all onnxruntime `
    --collect-data cv2 `
    --collect-binaries cv2 `
    $workerScript

Write-Host "Built OCR worker into $distRoot"
