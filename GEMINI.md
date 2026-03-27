# EGameTools & EGameSDK

A comprehensive mod menu and SDK for **Dying Light 2: Stay Human**, developed in C++.

## Project Overview

- **EGameSDK**: The core library that interfaces with the game's internal systems. It handles:
  - **Dynamic Reflection (CRTTI)**: A powerful system that traverses the engine's internal `CRTTI` metadata at runtime to find member offsets and class hierarchies by name. This allows for resilient and accurate property access without manual struct padding.
  - **Version-specific Offsets & Patterns**: Uses an `OffsetManager` to support multiple game versions (e.g., v1.12.0, v1.20.01).
  - **Hooking System**: A custom `HookBase` wrapper around MinHook for managing game function hooks.
  - **Game Internals**: Provides abstractions for game classes like `PlayerDI_PH`, `CGame`, `CInput`, etc.
- **EGameTools**: The end-user mod menu built on top of EGameSDK. Features include:
  - **ImGui UI**: A feature-rich menu supporting DX11 and DX12 renderers.
  - **Mod Features**: God Mode, FreeCam, Third Person, Weather Control, Inventory editing, etc.
  - **Custom Mod Loading**: Supports loading `.PAK` files and extracted game files from `UserModFiles`.
- **Ultimate-ASI-Loader-x64**: A third-party component used to load `EGameTools.asi` into the game process.

## Tech Stack

- **Language**: C++20/C++23 (`stdcpplatest`)
- **UI Framework**: ImGui
- **Hooking**: MinHook
- **Logging**: spdlog
- **Font Rendering**: FreeType
- **Graphics API**: DirectX 11 & DirectX 12
- **Build System**: Visual Studio 2022 (v143)

## Directory Structure

- `EGameSDK/`: Core library project.
  - `include/EGSDK/`: SDK headers (Offsets, Hooks, Game Classes).
  - `src/`: SDK implementation.
  - `deps/`: SDK-specific dependencies (MinHook, spdlog, steam).
- `EGameTools/`: Mod menu project.
  - `include/EGT/`: Menu headers (Menu tabs, Config, Renderer hooks).
  - `src/`: Menu implementation.
  - `deps/`: Menu-specific dependencies (ImGui, FreeType).
- `Ultimate-ASI-Loader-x64/`: ASI loader source code.
- `_IDAScripts/`: Python scripts for IDA Pro used during reverse engineering.

## Building and Running

### Build Instructions

1.  Open `EGameSDK.sln` in **Visual Studio 2022**.
2.  Set the configuration to **Release** and platform to **x64**.
3.  Build the solution. This will produce:
    - `x64\Release\EGameSDK.dll`
    - `x64\Release\EGameTools.asi`
4.  The `EGameTools.ini` config file is automatically copied to the output directory via a post-build event.

### Installation

1.  Copy `EGameSDK.dll`, `EGameTools.asi`, and `EGameTools.ini` to the game's executable directory:
    `Dying Light 2\ph\work\bin\x64`
2.  Rename `Ultimate-ASI-Loader-x64.dll` to `winmm.dll` (or any other supported name) and place it in the same directory.
3.  Launch the game. The menu can be toggled with **F5** by default.

## Development Conventions

### Initialization Flow
1.  **DllMain**: Initializes the logger and starts a `MainThread`.
2.  **MainThread**: 
    - Detects game version via `GameVersionCheck`.
    - Initializes `OffsetManager` with version-specific patterns.
    - Hooks the renderer (DX11/DX12) and game functions.
    - Loads/Initializes the configuration.

### Hooking
Use the `Hook` system defined in `EGameSDK/Utils/Hook.h`. New hooks should be added to `EGT::Engine::Hooks` or `EGSDK::GamePH::Hooks` depending on their scope.

### Offset Management
Patterns should be added to `EGameSDK/include/EGSDK/Offsets.h` and initialized in `EGameSDK/src/Offsets.cpp`. Always provide patterns for all supported game versions if possible.

### Logging
- Use `SPDLOG_INFO`, `SPDLOG_WARN`, `SPDLOG_ERROR` for logging.
- Logs are stored in `log.txt` (Tools) and `EGameSDK-log.txt` (SDK) in the game directory.

## Known Limitations
- Primarily designed for single-player; some features may not work or cause glitches in multiplayer.
- Game updates may break offsets; `OffsetManager` is designed to be easily updated with new patterns.
