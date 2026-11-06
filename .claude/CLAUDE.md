# CLAUDE.md

Environment and operating notes for Claude Code. Project rules live in the root `CLAUDE.md` (Japanese); read it first. This file is local-only (untracked).

## Hard rules

- **Do not run `git commit` or `git push` unless the user explicitly asks in the current turn.** Leave changes in the working tree and propose commit message/split instead. Earlier approvals do not carry over.
- Never rewrite history or force-push without explicit instruction. The remote history is unrelated to local history (see Git state).
- Do not claim anything unverified. Mark unchecked points as "unverified".

## Layout

- Git root: `Commander3D/`. UE project: `Proj/` (`Proj/Commander3D.uproject`). UE paths below are relative to `Proj/`.
- Tracked outside `Proj/`: `.gitignore`, `.gitattributes`, `CLAUDE.md`, `docs/`.
- Planned, not yet present: `Tests/Core/` (CMake+CTest), `Data/` (master data, location undecided).

## Modules

- Existing: `Commander3D` (Primary Game Module, `Source/Commander3D/`). Only `IMPLEMENT_PRIMARY_GAME_MODULE` + `CoreMinimal.h`. No classes, no `Content/` assets, no tests.
- Planned: `CommanderDomain` <- `CommanderApplication` <- `Commander3D`; `CommanderTests` (Editor-only).
- Domain/Application: standard C++ only. No `CoreMinimal.h`, `U/A/F` types, `TArray`, `FString`.
- When adding a module, register it in `Commander3D.uproject` `Modules` and in both `Source/*.Target.cs` `ExtraModuleNames` (tests: Editor target only).
- Details: `docs/architecture/overview.md`, `docs/domain/glossary.md`.

## Build environment

- Engine: Launcher (Installed Build) at `C:\Program Files\Epic Games\UE_5.8`, reported 5.8.4. `EngineAssociation` is a GUID.
- Toolchain: VS 18 Community, MSVC 14.51, Windows SDK 10.0.26100. `.vsconfig` is aligned with this (generic `VC.Tools`/`VC.ATL`, `Windows11SDK.26100`).
- Targets: `Commander3D` (Game), `Commander3DEditor` (Editor). `BuildSettingsVersion.V7`, `EngineIncludeOrderVersion.Unreal5_8`, C++20.
- Build verified: `Commander3DEditor Win64 Development` succeeds (~50 s).

```
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" Commander3DEditor Win64 Development "$PWD\Proj\Commander3D.uproject" -waitmutex
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "$PWD\Proj\Commander3D.uproject"
```

Tests (after `CommanderTests` exists, unverified): `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests <Filter>; Quit" -unattended -nullrhi`.

## Git state

- Branch `main`, no upstream. `origin` = https://github.com/nagisa2039/Commander3D (public, user's personal repo).
- `origin/main` (`0392931`) has **no common ancestor** with local `main`; its paths are `Commander3D/...` (no `Proj/`). A plain push will be rejected. History/push strategy is the user's decision.
- `SecurityToken` (Android file server) was removed from `DefaultEngine.ini` but remains in all earlier commits, local and (probably) remote.
- `*.sln` / `*.slnx` are untracked and ignored (generated). `.gitignore` anchors `/Proj/Build/`.
- `.gitattributes`: LF for source/config/data, CRLF for `.bat/.sln/.slnx`, LFS for UE binaries and common art/audio/video. LFS 3.7.1 installed; no files tracked by LFS yet.
- `.claude/` is intentionally untracked. Scratch dirs: `.claude/temp/`, `.claude/output/`.
- Conventional Commits. One commit = one rule or one vertical slice. Keep refactor / feature / format / binary changes separate. Commit trailer: `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## Config state

- `Config/DefaultEngine.ini`: still template defaults for `GameDefaultMap=/Engine/Maps/Templates/OpenWorld`, Lumen/RayTracing/Substrate enabled, DX12 SM6, `ActiveGameNameRedirects` from `TP_Blank`. No GameMode/startup map. Rendering features left as-is pending user decision.
- `Config/DefaultInput.ini`: Enhanced Input classes; no legacy action/axis mappings.
- `Config/DefaultGame.ini`: `ProjectID` only.

## Phase 0 progress

Done (committed): 0-1 token removal / untrack sln / `.gitignore`; 0-2 `.gitattributes`; 0-3 `.vsconfig` + unused config cleanup; 0-4 root `CLAUDE.md`, `docs/`.
Next: 0-5 `CommanderDomain` (empty + one UE-free class), 0-6 `Tests/Core` CMake + `GridCoord` test, 0-7 `Tools/CheckDomainPurity`, 0-8 `CommanderApplication`, 0-9 `CommanderTests`, 0-10 GitHub Actions.
Pending: Build.cs Public/Private dependency cleanup (do with 0-5); `GameDefaultMap` (after a custom map exists); Lumen/RayTracing/Substrate decision.

## Handover

Full background (Japanese): `.claude/temp/Commander3D_handover.md` (gitignored scratch).
