# cpp-practice

Minimal C++ CMake project scaffold for Windows (PowerShell).

Quick start (PowerShell):

```powershell
cd C:\DEV\practice\cpp-workspace
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Run the executable (if using Visual Studio generator the exe may be under `build/Debug`):

```powershell
# If using a single-config generator (Ninja/Make)
.\build\src\cpp_practice.exe

# If using Visual Studio generator (multi-config)
.\build\Debug\src\cpp_practice.exe
```

Recommended VS Code extensions:
- ms-vscode.cpptools
- ms-vscode.cmake-tools

