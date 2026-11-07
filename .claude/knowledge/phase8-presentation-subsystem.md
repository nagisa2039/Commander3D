# Phase 8f: Blueprint-facing campaign subsystem (2026-11-07)

UMG screens (`.uasset`) cannot be created by Claude (project rule 7). This is the C++ half they need: a thin Presentation layer the Widget Blueprints call. It holds no rules (rule 4).

## Added
- `Presentation/CommanderCampaignSubsystem.h` (`UCommanderCampaignSubsystem : UGameInstanceSubsystem`, module Commander3D):
  - run: `StartCampaign(name, bClassic)`, `SaveGame(slot)`, `LoadGame(slot)`, `HasCampaign`, `IsCampaignFinished`.
  - progress: `GetChapterName`, `GetChapterIndex`, `GetGold`.
  - preparation: `GetRoster` (`FCommanderUnitView[]`), `GetStock` (`FCommanderItemView[]`), `GetMaxDeployed`, `GetDeployedCount`, `Deploy`, `Withdraw`, `BuyItem`, `SellItem`, `EquipWeapon`, `TransferItem`, `DiscardItem`.
  - C++ only (the battle is not a Blueprint type): `StartBattle()` -> `std::optional<PreparedBattle>`, `CompleteBattle(battle, sortied)` -> `ECommanderCampaignStep`.
  - View types: `FCommanderItemView`, `FCommanderUnitView`, `ECommanderItemKind`, `ECommanderCampaignStep`.
- Paths: data `<Content>/Data`, saves `<Saved>/SaveGames/<slot>.json`.
- Blueprint input is validated here: negative ids / slots / entries are refused (never wrapped into a huge `size_t`).
- Test: `Commander3D.Presentation.CampaignSubsystem` (Automation 18/18).

## Changes elsewhere
- `Campaign` now creates a chapter's `Preparation` when the chapter begins (constructor / after `Complete`), not lazily, so a `const Preparation& Preparation() const` exists for read-only views. Behaviour is the same; `CampaignTest` unchanged.
- `FSaveStorage::Load` has an optional `FString* OutCampaignName` (the subsystem needs it to save a loaded run again).
- Build.cs: `CoreUObject` and `Engine` moved to Commander3D's Public dependencies (the subsystem header exposes them); `CommanderTests` gained `CoreUObject`, `Engine`.

## Notes
- A game-instance subsystem can only be created inside a `UGameInstance`; the test makes a bare `UGameInstance` as the outer (not initialised).
- Not done: the Widget Blueprints themselves (layout, buttons, lists) and anything that shows the battle map / chests; both need the editor. Wiring `StartBattle`/`CompleteBattle` to a battle presentation is also open.
