# Phase 0 config and project setup

date: 2026-11-07
commits: 9d0d3a6 ab738db d784e09 91ef279 b8bbd75

## Facts

| id | change | file | commit | verified |
|----|--------|------|--------|----------|
| K1 | Commander3D deps Core/CoreUObject/Engine/InputCore/EnhancedInput: Public -> Private | Proj/Source/Commander3D/Commander3D.Build.cs | 9d0d3a6 | build OK (Commander3DEditor Win64 Development) |
| K2 | add Private deps CommanderDomain, CommanderApplication to Commander3D | same | 9d0d3a6 | build OK |
| K3 | r.RayTracing=False; r.RayTracing.RayTracingProxies.ProjectEnabled=False; r.Substrate=False; remove r.Substrate.ProjectGBufferFormat | Proj/Config/DefaultEngine.ini | ab738db | editor behavior UNVERIFIED |
| K4 | keep Lumen GI/Reflection and Virtual Shadow Maps | Proj/Config/DefaultEngine.ini | - | - |
| K5 | add map /Game/Map/PL_Test (LFS tracked) | Proj/Content/Map/PL_Test.umap | b8bbd75 | LFS pointer confirmed via `git lfs ls-files` |
| K6 | GameDefaultMap and EditorStartupMap = /Game/Map/PL_Test.PL_Test | Proj/Config/DefaultEngine.ini | d784e09 | map load UNVERIFIED |
| K7 | plugin AndroidFileServer Enabled=false | Proj/Commander3D.uproject | 91ef279 | regeneration of ini section UNVERIFIED |
| K8 | EngineAssociation GUID -> "5.8" (editor rewrite, accepted) | Proj/Commander3D.uproject | 91ef279 | - |
| K9 | removed [/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings] (contains SecurityToken) | Proj/Config/DefaultEngine.ini | 91ef279 | - |

## Rationale

- K1/K2: no public header exists yet. Public only when a public header exposes the type (.claude/rules/module-layout.md).
- K3: SRPG does not need ray tracing or Substrate. Reduces shader cost and GPU requirement.
- K7/K9: editor regenerated the section; token was removed deliberately in 0-1. Disabling the plugin is the root fix.

## Pitfalls

- Editor rewrites Proj/.vsconfig (Windows11SDK.26100 -> 22621, adds VC.14.50 and Clang). Conflicts with 0-3. Fix: `git checkout Proj/.vsconfig`.
- Opening editor may regenerate DefaultEngine.ini sections. Check `git status` after each editor session.
- SecurityToken remains in history before 91ef279 and before 0-1. Not rewritten.
- First editor start after K3 may recompile shaders.

## Open items

- `.github/workflows/ci.yml` (0-10) was created, then disappeared; never committed. Recreate only on user request.
  - Design: job domain-purity (ubuntu: check_domain_purity.py + unittest), job core-tests (ubuntu+windows matrix: cmake -S Tests/Core, build, ctest).
  - Verified locally on MSVC only. GCC UNVERIFIED. UE build not in CI.
- UNVERIFIED: PL_Test opens on editor/game start; K7 no regeneration.
