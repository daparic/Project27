# Project27

A simple C++ Hello World project configured for step debugging in VS Code using the MSVC (`cl.exe`) toolchain from Visual Studio 2026 Community.

---

## Issues Encountered and Fixes Applied

### Issue 1 — Build task exited with code -1

**Error message:**
```
The preLaunchTask 'C/C++: cl.exe build active file' terminated with exit code -1
```

**Cause:**
The original `tasks.json` used task type `cppbuild` and invoked `cl.exe` directly. VS Code's integrated shell (PowerShell) does not inherit the Visual Studio developer environment, so `cl.exe` was not on `PATH` and the build failed immediately.

**Fix — `.vscode/tasks.json`:**

Changed the task type from `cppbuild` to `shell` using `cmd.exe /C` as the shell executor. The command uses `call` to invoke `VsDevCmd.bat` (which sets up the full MSVC environment), then chains `cl.exe` with `&&`.

Two quoting details matter here:
- `call` is required before the `.bat` path so that `cmd.exe` properly inherits the environment after the batch file returns.
- The output path flag is written as `/Fe"<path>"` (quote after the flag name, not around the whole token), which is what `cl.exe` expects.

```json
{
    "type": "shell",
    "label": "C/C++: cl.exe build active file",
    "command": "call \"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\Common7\\Tools\\VsDevCmd.bat\" && cl.exe /Zi /EHsc /nologo /Fe\"${fileDirname}\\${fileBasenameNoExtension}.exe\" \"${file}\"",
    "options": {
        "shell": {
            "executable": "cmd.exe",
            "args": ["/C"]
        },
        "cwd": "${fileDirname}"
    },
    "problemMatcher": ["$msCompile"],
    "group": {
        "kind": "build",
        "isDefault": true
    }
}
```

---

### Issue 2 — Debugger reported program does not exist

**Error message:**
```
launch: program 'C:\Users\user1\source\repos\Project27\Project27\main.exe' does not exist
```

**Cause:**
This was a downstream consequence of Issue 1. Because the build task failed, `main.exe` was never produced, so the debugger had nothing to launch.

Once Issue 1 is fixed and the build succeeds, `cl.exe` writes the output binary to `${fileDirname}\${fileBasenameNoExtension}.exe` (e.g., `Project27\Project27\main.exe`), which matches the `program` path already set in `launch.json`.

No path change was required in `launch.json`.

---

### Issue 3 — Spurious non-standard fields in `launch.json`

**Cause:**
The auto-generated `launch.json` contained several fields that are not part of the `cppvsdbg` launch configuration schema:

| Field | Status |
|---|---|
| `existing` | Not a valid `cppvsdbg` field |
| `detail` | Not a valid `cppvsdbg` field |
| `taskDetail` | Not a valid `cppvsdbg` field |
| `taskStatus` | Not a valid `cppvsdbg` field |
| `isDefault` | Belongs on compound configurations, not individual ones |

**Fix — `.vscode/launch.json`:**

Removed all non-standard fields, leaving a clean configuration:

```json
{
    "name": "C/C++: cl.exe build and debug active file",
    "type": "cppvsdbg",
    "request": "launch",
    "program": "${fileDirname}\\${fileBasenameNoExtension}.exe",
    "args": [],
    "stopAtEntry": false,
    "cwd": "${fileDirname}",
    "environment": [],
    "console": "integratedTerminal",
    "preLaunchTask": "C/C++: cl.exe build active file"
}
```

---

## Compiler Toolchain

When you press `F5`, the build task calls `VsDevCmd.bat` and then invokes `cl.exe`. The exact toolchain in use is:

| Property | Value |
|---|---|
| Compiler | Microsoft (R) C/C++ Optimizing Compiler |
| Compiler version | 19.50.35730 |
| MSVC toolset | 14.50.35717 |
| Visual Studio | 2026 Community (version 18.5.2) |
| `cl.exe` path | `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx86\x86\cl.exe` |
| Target architecture | **x86 (32-bit)** — default when `VsDevCmd.bat` is called without `-arch` |
| Debug info format | PDB (produced by the `/Zi` flag passed to `cl.exe`) |
| Debugger engine | `cppvsdbg` (Visual Studio debugger, reads PDB files) |

### Architecture note

`VsDevCmd.bat` defaults to the **x86** toolchain when no `-arch` argument is given. The existing Visual Studio project (`Project27.vcxproj`) targets **x64**, so there is a mismatch. To build a 64-bit executable from the VS Code task, change the `call` line in `.vscode/tasks.json` to:

```
call "...VsDevCmd.bat" -arch=x64 && cl.exe ...
```

The x64 `cl.exe` lives at:
`C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe`
and reports itself as **Version 19.50.35730 for x64**.

---

## Required VS Code Extension

Only **one** extension is needed to step debug this project:

| Extension | Publisher | Version | Role |
|---|---|---|---|
| C/C++ (`ms-vscode.cpptools`) | Microsoft | 1.32.2 | Registers the `cppvsdbg` debugger type used in `launch.json` |

### Why only one

The build task (`tasks.json`) uses `"type": "shell"` — a built-in VS Code task type that needs no extension. VS Code passes the command directly to `cmd.exe`. The debugger (`launch.json`) uses `"type": "cppvsdbg"`, which is registered solely by `ms-vscode.cpptools`. Remove any other extension and debugging still works; remove `ms-vscode.cpptools` and the debug session cannot start.

```
F5
 ├── preLaunchTask  →  "type": "shell"     →  built-in VS Code     →  no extension needed
 └── launch         →  "type": "cppvsdbg"  →  ms-vscode.cpptools   →  required
```

### Extensions that are NOT required

| Extension | Why not needed |
|---|---|
| `vscode-solution-explorer` (Fernando Escolar) | UI panel for browsing `.sln`/`.slnx` trees — no role in building or debugging |
| `ms-vscode.cpptools-extension-pack` | Meta-pack that installs `cpptools` + themes + CMake together; not itself functional |
| `ms-vscode.cpptools-themes` | Syntax colour themes only |
| `ms-vscode.cmake-tools` | CMake build integration — project uses MSBuild/`cl.exe`, not CMake |

---

## Environment

| Component | Details |
|---|---|
| IDE | Visual Studio Code |
| Visual Studio | 2026 Community (`C:\Program Files\Microsoft Visual Studio\18\Community`) |
| Platform | Windows 10 x64 |
| Debugger type | `cppvsdbg` |

> **Note:** Visual Studio 2026 was reported by `vswhere` as `isRebootRequired: true` at the time of this fix. The C++ tools are present and functional, but a system reboot is recommended to let the installer finalize.

---

## How to Debug

1. Open `Project27\main.cpp` in VS Code so it is the active file.
2. Press `F5` (or **Run > Start Debugging**).
3. The `preLaunchTask` compiles the file using `cl.exe` (x86, 32-bit by default) via the VS 2026 developer environment.
4. The debugger launches the resulting `main.exe` and stops at breakpoints.
