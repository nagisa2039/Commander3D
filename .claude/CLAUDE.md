# CLAUDE.md

Environment and operating notes for Claude Code. Project rules live in the root `CLAUDE.md` (Japanese); read it first. This file is local-only (untracked).

## Hard rules

- **Do not run `git commit` or `git push` unless the user explicitly asks in the current turn.** Leave changes in the working tree and propose commit message/split instead. Earlier approvals do not carry over.
- Never rewrite history or force-push without explicit instruction. The remote history is unrelated to local history (see Git state).
- Do not claim anything unverified. Mark unchecked points as "unverified".
- Prefer the dedicated tools (Read / Write / Edit / Glob / Grep) for reading, writing, creating, and searching files. Do not use shell commands (`cat`, `sed -i`, `echo >`, heredocs, `python` one-liners, etc.) for file operations unless the dedicated tools cannot do the job.

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

- Branch `main`, upstream `origin/main`. `origin` = https://github.com/nagisa2039/Commander3D (public, user's personal repo).
- 2026-11-07: local history was rewritten and force-pushed over `origin/main` (old `0392931`, paths `Commander3D/...`) at user's explicit request. Old remote commit may remain reachable by SHA on GitHub (UNVERIFIED). All commit hashes before that date changed; do not trust older hash references.
- `SecurityToken=` lines were purged from all commits on `main`; generated dirs (`Proj/Intermediate|Saved|Binaries|DerivedDataCache|.vs`) were purged too (a >100MB file blocked the push). Never force-push or rewrite history again without explicit instruction.
- `*.sln` / `*.slnx` are untracked and ignored (generated). `.gitignore` anchors `/Proj/Build/`.
- `.gitattributes`: LF for source/config/data, CRLF for `.bat/.sln/.slnx`, LFS for UE binaries and common art/audio/video. LFS 3.7.1 installed; `Proj/Content/Map/PL_Test.umap` is the first LFS-tracked file.
- `.claude/` is tracked except scratch dirs (gitignored): `.claude/temp/`, `.claude/output/`.
- Conventional Commits. One commit = one rule or one vertical slice. Keep refactor / feature / format / binary changes separate. Commit trailer: `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.

## Config state

- `Config/DefaultEngine.ini`: `GameDefaultMap`/`EditorStartupMap` = `/Game/Map/PL_Test.PL_Test`; RayTracing and Substrate off; Lumen GI/Reflection and Virtual Shadow Maps on; DX12 SM6; `ActiveGameNameRedirects` from `TP_Blank`.
- `Commander3D.uproject`: plugin `AndroidFileServer` disabled (editor regenerated its settings incl. SecurityToken).
- `Config/DefaultInput.ini`: Enhanced Input classes; no legacy action/axis mappings.
- `Config/DefaultGame.ini`: `ProjectID` only.
- The editor rewrites `Proj/.vsconfig` and may regenerate ini sections; check `git status` after editor sessions.

## Progress

Phase 0 done except 0-10 (GitHub Actions; `ci.yml` was created then lost, not committed).
Phase 1 done except 3D display: `Grid`, `BattleMap`, `MapLoader`, `FMapJsonParser` (JSON -> `MapData`). Master data lives in `Proj/Content/Data/` (maps: `Data/Maps/*.json`).
Details: `.claude/knowledge/`.
Rule: after implementing, run CTest AND the UE build + Automation (`Commander3D.Wiring`). Commit messages in Japanese; add knowledge at commit time.

## Handover

Full background (Japanese): `.claude/temp/Commander3D_handover.md` (gitignored scratch).
