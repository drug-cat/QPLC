# Contributing to QPLC

Thanks for your interest in QPLC! This document explains how to contribute effectively.

## Code of Conduct

By participating, you agree to abide by the [Code of Conduct](CODE_OF_CONDUCT.md).

## How to contribute

### Reporting bugs

Open an issue with:
- Minimal `.q` reproducer
- Expected vs actual output
- Compiler version (`qplc --version` — coming soon)
- OS + compiler (`g++ --version` or `clang --version`)

### Proposing features

Open an issue with the `enhancement` label. Discuss before implementing.

### Submitting pull requests

1. Fork the repo and create a feature branch: `git checkout -b feat/my-change`
2. Make focused, atomic commits with clear messages
3. Add tests for new features (place in `tests/run_tests.sh`)
4. Ensure all 32 tests pass: `bash tests/run_tests.sh`
5. Build all projects: `dotnet build QPLC.slnx` and `cmake --build build`
6. Update `CHANGELOG.md` with your change
7. Open the PR with a clear description

## Development setup

### C++ compiler (Windows)

```bash
export PATH="/c/msys64/ucrt64/bin:$PATH"
cmake -S . -B build -G Ninja
cmake --build build
```

### C++ compiler (Linux)

```bash
sudo apt install g++ cmake ninja-build
cmake -S . -B build -G Ninja
cmake --build build
```

### .NET projects

```bash
dotnet build QPLC.slnx
```

## Coding style

### C++

- C++20, no exceptions in hot paths
- 4‑space indent, snake_case for variables/functions, PascalCase for types
- Prefer `const`, references, `std::unique_ptr` over raw pointers
- Add inline comments only for non‑obvious constraints (not narrating the code)

### C#

- .NET 8, `Nullable enable`, latest C# language version
- PascalCase for public APIs, _camelCase for private fields
- One public type per file

## Testing

- **Unit tests:** `bash tests/run_tests.sh` — must show 32/32 PASS
- **Golden test:** `output.xml` from `test_all.q` is committed and must be byte‑identical
- **Manual integration:** load each example in QPLC.Studio, run for 10 scans, verify trace

## Release process

1. Update `CHANGELOG.md` with version + date
2. Bump version in `CMakeLists.txt`, `vscode/package.json`, all `.csproj`
3. Tag: `git tag v0.X.0`
4. Build release artifacts (`cmake --build build --config Release`)
5. Push tag: `git push origin v0.X.0`
6. GitHub Actions builds + uploads artifacts
