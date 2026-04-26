# ImGui local changes (EGameTools)

## Baseline snapshot (exact upstream match)

Vendored core ImGui is based on upstream commit:

- `8a14b71f2284bbc13fd2900ecefe3f346618702b`
- Commit title: `Version 1.90.4 WIP`
- Version macros at that commit: `IMGUI_VERSION "1.90.4 WIP"`, `IMGUI_VERSION_NUM 19031`

Compared against that exact commit, EGameTools carries upstream changes only via the patch set below (plus any files you add alongside ImGui, e.g. `imguiex`).

1. `imgui_widgets.cpp` — patch `001` (`Selectable()` rounded/nav tweak)
2. `imconfig.h` — patch `002` (`IMGUI_ENABLE_FREETYPE` enabled)
3. `backends/imgui_impl_dx12.cpp/.h` — patch `003` (font atlas upload on app command queue; avoids `CreateCommandQueue` per rebuild for injected DX12)

## Patches (re-apply after replacing `deps/ImGui` from upstream)

- `001-selectable-rounded-nav.patch`
  - file: `imgui_widgets.cpp`
  - purpose: rounds `Selectable()` header background and keeps nav highlight rounded (drops `ImGuiNavHighlightFlags_NoRounding`)
- `002-enable-freetype.patch`
  - file: `imconfig.h`
  - purpose: enables `#define IMGUI_ENABLE_FREETYPE` automatically
- `003-dx12-font-upload-shared-queue.patch`
  - files: `backends/imgui_impl_dx12.cpp`, `backends/imgui_impl_dx12.h`
  - purpose: `ImGui_ImplDX12_SetFontUploadCommandQueue()` + upload path using the game’s queue (EGameTools sets this from the Present path). Stock backend creates a temporary queue each font rebuild, which can crash some titles after menu scale / atlas rebuild.

### Apply patches

From `EGameTools/deps/ImGui`:

```bat
cd EGameTools\deps\ImGui
git apply --check patches\001-selectable-rounded-nav.patch
git apply --check patches\002-enable-freetype.patch
git apply patches\001-selectable-rounded-nav.patch
git apply patches\002-enable-freetype.patch
REM Patch 003: apply from repo root (git apply inside deps\ImGui incorrectly skips this patch on some Git versions).
cd ..\..\..
git apply --check --directory=EGameTools/deps/ImGui EGameTools/deps/ImGui/patches/003-dx12-font-upload-shared-queue.patch
git apply --directory=EGameTools/deps/ImGui EGameTools/deps/ImGui/patches/003-dx12-font-upload-shared-queue.patch
```

Or run the helper:

```bat
cd EGameTools\deps\ImGui\patches
apply-local-imgui-patches.bat
```

### FreeType options

- **Automatic (recommended):** apply `patches/002-enable-freetype.patch`
- **Manual:** edit `imconfig.h` and uncomment:
  - `//#define IMGUI_ENABLE_FREETYPE`

If patch `002` fails on future versions, the line/block likely moved; manually enable it or refresh the patch hunk.

### Files that are not upstream ImGui

Keep these when updating:

- `imguiex.cpp`, `imguiex.h`
- `imguiex_animation.*`
- `imgui_hotkey.*`
- project-specific assets under `deps/ImGui`

### Suggested upgrade workflow

1. Replace upstream files from the branch/commit you want.
2. Re-apply `001`, `002`, and `003` patches (refresh hunks if upstream moved lines).
3. Reconcile `imguiex` / `imgui_hotkey` if API changed.
4. Build and fix compile errors.
