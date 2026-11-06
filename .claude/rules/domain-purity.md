---
paths:
  - "Proj/Source/CommanderDomain/**"
---

# CommanderDomain rules

- Game rules (damage, movement, turn management) live here, not in Actors/Components/Blueprints.
- No UE types: no `CoreMinimal.h`, `UObject`, `UCLASS/USTRUCT/UENUM`, `TArray`, `FString`. Use the C++ standard library.
- `CommanderDomain.Build.cs` must not depend on Engine/CoreUObject.
- Must be testable from `CommanderTests` without a World.
