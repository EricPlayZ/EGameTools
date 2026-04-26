@echo off
setlocal

REM Run this script from EGameTools\deps\ImGui\patches
REM or from EGameTools\deps\ImGui.

set "ROOT=%~dp0.."
if exist "%~dp0\001-selectable-rounded-nav.patch" (
    set "PATCH_DIR=%~dp0"
) else if exist "%ROOT%\patches\001-selectable-rounded-nav.patch" (
    set "PATCH_DIR=%ROOT%\patches\"
) else (
    echo [ERROR] Could not locate patch files.
    echo         Expected 001-selectable-rounded-nav.patch near this script.
    exit /b 1
)

set "P1=%PATCH_DIR%001-selectable-rounded-nav.patch"
set "P2=%PATCH_DIR%002-enable-freetype.patch"
set "P3=%PATCH_DIR%003-dx12-font-upload-shared-queue.patch"

echo [INFO] Using patch directory:
echo        %PATCH_DIR%
echo.

pushd "%ROOT%" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Could not cd to ImGui root: %ROOT%
    exit /b 1
)

echo [STEP] Patch 001 (selectable rounded nav)...
git apply --check "%P1%" >nul 2>&1
if errorlevel 1 (
    git apply --reverse --check "%P1%" >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Patch 001 does not apply and is not already applied.
        echo         Verify imgui_widgets.cpp baseline matches patch context.
        popd
        exit /b 1
    )
    echo        Already applied — skipping.
) else (
    git apply "%P1%"
    if errorlevel 1 (
        echo [ERROR] Failed applying patch 001.
        popd
        exit /b 1
    )
    echo        Applied.
)

echo [STEP] Patch 002 (FreeType)...
git apply --check "%P2%" >nul 2>&1
if errorlevel 1 (
    git apply --reverse --check "%P2%" >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Patch 002 does not apply and is not already applied.
        echo         Enable IMGUI_ENABLE_FREETYPE manually in imconfig.h if needed.
        popd
        exit /b 1
    )
    echo        Already applied — skipping.
) else (
    git apply "%P2%"
    if errorlevel 1 (
        echo [ERROR] Failed applying patch 002.
        popd
        exit /b 1
    )
    echo        Applied.
)

popd

REM Patch 003 touches backends\*. git apply from ImGui dir incorrectly "skips" this patch on some setups;
REM apply from repo root with --directory (paths in patch are backends\...).
pushd "%ROOT%\..\..\.." >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Could not cd to repo root from %ROOT%\..\..\..
    exit /b 1
)

echo [STEP] Patch 003 (DX12 font upload queue)...
git apply --check --directory=EGameTools/deps/ImGui "%P3%" >nul 2>&1
if errorlevel 1 (
    git apply --reverse --check --directory=EGameTools/deps/ImGui "%P3%" >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Patch 003 does not apply and is not already applied.
        echo         Verify backends/imgui_impl_dx12.* baseline matches patch.
        popd
        exit /b 1
    )
    echo        Already applied — skipping.
) else (
    git apply --directory=EGameTools/deps/ImGui "%P3%"
    if errorlevel 1 (
        echo [ERROR] Failed applying patch 003.
        popd
        exit /b 1
    )
    echo        Applied.
)

popd

echo.
echo [OK] Applied local ImGui patches successfully.
exit /b 0
