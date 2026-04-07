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

echo [INFO] Using patch directory:
echo        %PATCH_DIR%
echo.

echo [STEP] Checking patch 001...
git apply --check "%P1%"
if errorlevel 1 (
    echo [ERROR] Patch 001 check failed.
    echo         If this is expected, verify imgui_widgets.cpp baseline and patch context.
    exit /b 1
)

echo [STEP] Checking patch 002...
git apply --check "%P2%"
if errorlevel 1 (
    echo [ERROR] Patch 002 check failed.
    echo         If FreeType is already enabled, this is expected.
    echo         You can enable it manually in imconfig.h if needed.
    exit /b 1
)

echo [STEP] Applying patch 001...
git apply "%P1%"
if errorlevel 1 (
    echo [ERROR] Failed applying patch 001.
    exit /b 1
)

echo [STEP] Applying patch 002...
git apply "%P2%"
if errorlevel 1 (
    echo [ERROR] Failed applying patch 002.
    exit /b 1
)

echo.
echo [OK] Applied local ImGui patches successfully.
exit /b 0
