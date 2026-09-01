# Windows launcher notes - deferred

The optional launcher is Linux-first for now. Do not alter the existing Windows V_mini_me runtime while the launcher behavior is still being validated.

When a Windows launcher is implemented, keep the same user-facing config keys and launcher behavior but replace the POSIX process/control pieces:

- `readlink("/proc/self/exe")` -> `GetModuleFileNameW()`.
- `access(..., X_OK)` -> `GetFileAttributesW()` / normal executable existence checks.
- `fork()` + `exec()` -> `CreateProcessW()`.
- POSIX anonymous `pipe()` -> `CreatePipe()` with inheritable child stdin handle.
- `waitpid(..., WNOHANG)` -> `WaitForSingleObject(process, 0)` / `GetExitCodeProcess()`.
- POSIX file descriptors for runtime commands -> Windows pipe `HANDLE` writes.
- `/dev/null` -> `NUL`.
- `setsid()` is not needed; use appropriate `CREATE_NEW_PROCESS_GROUP` / detached flags only where useful.

Keep the launcher a separate optional executable beside `V_mini_me.exe`. Do not add raylib as a dependency of the core Windows runtime.

The Linux implementation intentionally talks to the core through its existing terminal commands (`e`, slot number, `r`, `q`) rather than adding an IPC subsystem. Reuse that strategy on Windows first unless it proves unreliable.
