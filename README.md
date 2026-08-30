<p align="center">
	<b>NayClient</b>
	<br>
	<b>A lightweight, injectable utility client for Minecraft: Bedrock Edition written in C</b>
</p>

<p align="center">
	<img src="https://img.shields.io/static/v1?label=platform&message=Windows%20x64&color=0078D6&logo=windows&logoColor=white" alt="Platform" />
	<img src="https://img.shields.io/static/v1?label=language&message=C11&color=555555&logo=c&logoColor=white" alt="Language" />
	<img src="https://img.shields.io/static/v1?label=build&message=CMake%20%2B%20MSVC&color=064F8C&logo=cmake&logoColor=white" alt="Build" />
	<img src="https://img.shields.io/static/v1?label=minecraft&message=Bedrock%20x64&color=12c970&logo=minecraft&logoColor=white" alt="Minecraft" />
</p>

## What is this?

NayClient is a small, injectable utility client for **Minecraft: Bedrock Edition (Windows x64)**, written from scratch in **C**. It ships as a DLL that is injected into the game and a console launcher that drives it. We do not take responsibility for anything done with this software.

If you want a minimal, readable base to build Bedrock modules on — without a heavy C++ SDK — look no further.

- 🧩 **Tiny module framework** — register enable/disable/tick callbacks, toggle at runtime
- 🎛️ **Console launcher** — version-gated injection, colored status, live command loop
- ♻️ **Clean unload** — restores every patch and frees the library on exit

## How it works

NayClient follows the same model as other Bedrock clients: **per-version signatures and offsets** anchored into the running module.

1. **Launcher** (`naylauncher.exe`) checks the Minecraft version, then injects `nayclient.dll` via `VirtualAllocEx` + `WriteProcessMemory` + `CreateRemoteThread(LoadLibraryW)`.
2. **Client** (`nayclient.dll`) spins a worker thread that waits on named events (`Local\NayClient.*.<pid>`) for toggles and unload.
3. **Modules** resolve their targets in the live process — a known **vtable RVA** (Fullbright), an **AOB signature** (NoFire), or an **inline hook** on the kill function (AutoGG) — then apply and cleanly revert their patches.

## Building

Requirements: **CMake 3.20+**, a **Windows x64** toolchain (**MSVC** recommended; MinGW-w64 works).

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Outputs:
- `nayclient.dll` — the injectable client
- `naylauncher.exe` — the launcher (expects `nayclient.dll` next to it)

## Usage

```
naylauncher.exe        # start Minecraft first, then run the launcher
help                   # list commands
status                 # client and module state
fullbright             # toggle Fullbright
nofire                 # toggle NoFire
quit                   # disable modules and unload
```

## Project layout

```
include/nay/           public headers
src/client.c           DllMain, worker thread, client lifecycle
src/launcher/          console launcher + injector
src/modules/           module framework + fullbright / nofire / autogg
src/platform/          Minecraft version detection
```

## Disclaimer

NayClient is not affiliated with Mojang or Microsoft. All brands and trademarks belong to their respective owners. It is not a Mojang-approved product and is not associated with Mojang. Use it at your own risk, only on servers and in modes where you are permitted to.

## Licensing

This project is provided as-is for educational and research purposes. See the `LICENSE` file for details.
