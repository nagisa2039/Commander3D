# Phase 0 config and project setup

date: 2026-11-07
commits: bb1cb21 ead059e 359a7cf 8f953cf 90aca80

## Facts

| id | change | file | commit | verified |
|----|--------|------|--------|----------|
| K1 | Commander3D deps Core/CoreUObject/Engine/InputCore/EnhancedInput: Public -> Private | Proj/Source/Commander3D/Commander3D.Build.cs | bb1cb21 | build OK (Commander3DEditor Win64 Development) |
| K2 | add Private deps CommanderDomain, CommanderApplication to Commander3D | same | bb1cb21 | build OK |
| K3 | r.RayTracing=False; r.RayTracing.RayTracingProxies.ProjectEnabled=False; r.Substrate=False; remove r.Substrate.ProjectGBufferFormat | Proj/Config/DefaultEngine.ini | ead059e | editor behavior UNVERIFIED (Cmd run loaded without error) |
| K4 | keep Lumen GI/Reflection and Virtual Shadow Maps | Proj/Config/DefaultEngine.ini | - | - |
| K5 | add map /Game/Map/PL_Test (LFS tracked) | Proj/Content/Map/PL_Test.umap | 90aca80 | LFS pointer confirmed via `git lfs ls-files` |
| K6 | GameDefaultMap and EditorStartupMap = /Game/Map/PL_Test.PL_Test | Proj/Config/DefaultEngine.ini | 359a7cf | verified: UnrealEditor-Cmd log shows MAP LOAD of Content/Map/PL_Test.umap at startup (2026-11-07) |
| K7 | plugin AndroidFileServer Enabled=false | Proj/Commander3D.uproject | 8f953cf | after one UnrealEditor-Cmd run, git status clean (no regeneration); full editor session UNVERIFIED |
| K8 | EngineAssociation GUID -> "5.8" (editor rewrite, accepted) | Proj/Commander3D.uproject | 8f953cf | - |
| K9 | removed [/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings] (contains SecurityToken) | Proj/Config/DefaultEngine.ini | 8f953cf | - |

## Rationale

- K1/K2: no public header exists yet. Public only when a public header exposes the type (.claude/rules/module-layout.md).
- K3: SRPG does not need ray tracing or Substrate. Reduces shader cost and GPU requirement.
- K7/K9: editor regenerated the section; token was removed deliberately in 0-1. Disabling the plugin is the root fix.

## Pitfalls

- Editor rewrites Proj/.vsconfig (Windows11SDK.26100 -> 22621, adds VC.14.50 and Clang). Conflicts with 0-3. Fix: `git checkout Proj/.vsconfig`.
- Opening editor may regenerate DefaultEngine.ini sections. Check `git status` after each editor session.
- History rewritten 2026-11-07 (user instruction): SecurityToken lines (3 distinct values, incl. one already public on origin/main old 0392931) and generated dirs removed from all commits; tip tree identical to pre-rewrite; force-pushed (--force-with-lease). Hashes in this file are post-rewrite.
- Old public token B9F4...: Android File Server (USB-only) token; need to rotate: UNVERIFIED. Old remote commit reachable by SHA on GitHub: UNVERIFIED.
- Local branch backup/pre-token-purge holds the pre-rewrite history (contains tokens and large files); never push it; delete when no longer needed.
- First editor start after K3 may recompile shaders.

## Open items

- `.github/workflows/ci.yml` (0-10) was created, then disappeared; never committed. Recreate only on user request.
  - Design: job domain-purity (ubuntu: check_domain_purity.py + unittest), job core-tests (ubuntu+windows matrix: cmake -S Tests/Core, build, ctest).
  - Verified locally on MSVC only. GCC verification not required (user decision 2026-11-07); ubuntu/GCC matrix entry is optional. UE build not in CI.
- UNVERIFIED: K7 no regeneration across a full (GUI) editor session.
