# Installation

QPLC ships as a C++20 command-line compiler (`qplc`) plus a set of .NET 8 simulators and an Avalonia IDE. Building from source is the only way to get started today.

## Prerequisites

- **CMake** ≥ 3.16
- **A C++20 compiler** — GCC ≥ 11 or Clang ≥ 14 (MSVC also works)
- **Ninja** build system (recommended)
- **.NET SDK 8.0** for the simulators and Studio

### Windows (MSYS2 UCRT64)

Install [MSYS2](https://www.msys2.org/), then in the **UCRT64** environment:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-cmake
```

If you need the VS Code extension, install the .NET SDK from [dotnet.microsoft.com](https://dotnet.microsoft.com/).

### Linux (Debian/Ubuntu)

```bash
sudo apt install build-essential cmake ninja-build dotnet-sdk-8.0
```

### macOS

```bash
brew install cmake ninja gcc
brew install --cask dotnet-sdk
```

## Build the compiler

```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"   # Windows/MSYS2 only
cmake -S . -B build -G Ninja
cmake --build build
```

This produces:

- `build/qplc.exe` (Windows) or `build/qplc` (Linux/macOS)

## Build the simulators

```bash
dotnet build QPLCSimulator/QPLCSimulator.csproj
dotnet build QPLC.Studio/QPLC.Studio.csproj
```

## Sanity check

```bash
./build/qplc examples/conf.qplc examples/test_all.q -o output.xml
dotnet QPLCSimulator/bin/Debug/net8.0/QPLCSimulator.dll examples/conf.qplc output.xml
```

You should see the REPL prompt. Type `show` and press Enter to list all variables.

## CI-ready

The repository includes GitHub Actions workflows for Windows (MSYS2), Ubuntu, and .NET. See [`.github/workflows/`](https://github.com/YOUR_USERNAME/QPLC/tree/main/.github/workflows) for details.
