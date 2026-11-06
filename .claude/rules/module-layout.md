---
paths:
  - "Proj/Source/**/*.Build.cs"
  - "Proj/Source/*.Target.cs"
  - "Proj/Commander3D.uproject"
---

# Module wiring

- Dependency direction: Commander3D -> CommanderApplication -> CommanderDomain. Never the reverse.
- New modules must be registered in `Commander3D.uproject` `Modules` and in both `Source/*.Target.cs` `ExtraModuleNames`.
- `CommanderTests` is Editor-only.
- Prefer PrivateDependencyModuleNames; use Public only when headers expose the dependency.
