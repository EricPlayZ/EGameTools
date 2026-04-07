# ImGui local changes (EGameTools)

## Baseline snapshot (exact upstream match)

Vendored core ImGui is based on upstream commit:

- `8a14b71f2284bbc13fd2900ecefe3f346618702b`
- Commit title: `Version 1.90.4 WIP`
- Version macros at that commit: `IMGUI_VERSION "1.90.4 WIP"`, `IMGUI_VERSION_NUM 19031`

Compared against that exact commit, EGameTools has only 2 intentional deltas:

1. `imgui_widgets.cpp` (`Selectable()` rounded/nav tweak)
2. `imconfig.h` (`IMGUI_ENABLE_FREETYPE` enabled)

## Patches (re-apply after replacing `deps/ImGui` from upstream)

- `001-selectable-rounded-nav.patch`
  - file: `imgui_widgets.cpp`
  - purpose: rounds `Selectable()` header background and keeps nav highlight rounded (drops `ImGuiNavHighlightFlags_NoRounding`)
- `002-enable-freetype.patch`
  - file: `imconfig.h`
  - purpose: enables `#define IMGUI_ENABLE_FREETYPE` automatically

### Apply patches

From `EGameTools/deps/ImGui`:

```bat
git apply --check patches\001-selectable-rounded-nav.patch
git apply --check patches\002-enable-freetype.patch
git apply patches\001-selectable-rounded-nav.patch
git apply patches\002-enable-freetype.patch
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
2. Re-apply `001` and `002` patches.
3. Reconcile `imguiex` / `imgui_hotkey` if API changed.
4. Build and fix compile errors.
