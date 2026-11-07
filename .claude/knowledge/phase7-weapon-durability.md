# Phase 7a: weapon durability

Phase 7 (class / XP / level-up / items / durability) is split into slices; this is the first. XP/level-up, classes, items are NOT done.

## Added
- `WeaponSpec::Durability` (max uses; 0 = never breaks; default 0, so existing data/tests behave as before). `BattleUnit::WeaponUses` (uses left).
- `HasUsableWeapon(unit)` = `MaxRange > 0 && (Durability == 0 || WeaponUses > 0)`; `WeaponReaches` now requires it. The AI uses `WeaponReaches`, so it never plans an attack with a broken weapon.
- `Battle::Resolve` (private): each strike (hit OR miss) with a durable weapon does `--WeaponUses`; `Strike::WeaponBroke` is true when it reaches 0. Applies to counters too.
- `Battle::AddUnit` also rejects `Durability < 0` and, for durable weapons, `WeaponUses` outside `0..Durability` (0 = already broken is allowed).
- Infrastructure: weapon JSON `"durability"` (optional int, via new `JsonRead::ReadOptionalInt32Field`); `FBattleJsonParser` starts a unit with `WeaponUses = Durability`. Sample `Weapons.json`: sword 40, bow 30 (placeholder numbers).
- Tests: CombatTest +7 cases (CTest 13/13); `Commander3D.Json.*` extended (Automation 13/13).

## Decisions
- W1: one use per strike, hit or miss (first-FE style). Not per attack action, so attacker and counter-attacker each lose one.
- W2: a broken weapon is not removed or replaced: the unit just cannot attack/counter (MaxRange stays). No repair, no item swap yet (items are a later slice).
- W3: `WeaponUses == 0` is only "broken" when `Durability > 0`; for Durability 0 it is ignored (the field defaults to 0 for such units).
- W4: JSON has no per-unit uses override (YAGNI; units start with fresh weapons).
- W5: the sample weapon numbers are placeholders, not tuned.

## Update 3: enemy healers, staff XP, promotion choice + seals, avoid/rank hit bonuses, strict `passable` (2026-11-07)

User clarification: enemies do **not** use items, but enemy **healers** exist. So the AI uses staffs only; potions / repair kits / seals stay player-only.

### Added
- **Enemy healer AI** (`EnemyAi.h`): a unit with a staff first heals (before attacking): among allies on its side (not itself) that `NeedsHealing` (Hp <= 75% of MaxHp, `HealBelowPercent`) and are within staff `Range` of a cell it can stand on, it picks the lowest Hp share, then lowest Hp, then cheaper move, then lower id; it moves into range first if needed (`UseStaffCommand` next call). If there is nothing to heal: an armed healer fights as usual; an **unarmed** healer (staff, no usable weapon) walks toward its nearest hurt ally (path cost, via the generalized `PathCostFields` / `ApproachCell(B, Unit, IsTarget)`) instead of the player's units, and waits when nobody is hurt. Enemies never use potions / repair kits (tested: no `ItemUsed` events).
- **Staff XP**: `UseStaff(healer, target, slot, Rng)` (Rng added) — a player healer below MaxLevel earns `CalculateStaffExperience` = 8 + HpHealed/2; `StaffUsed` carries optional `Experience` and `LevelUp`. Enemy healers earn nothing.
- **Promotion choice**: `BattleUnit::Promotions` (vector, max `MaxPromotionOptions` = 4) replaces the single optional promotion; `PromotionSpec::Name`. `PromoteUnit(unit, option, sealSlot = nullopt)`; taking any option clears the rest. `UnitPromoted` has `Option`, `Name`, `UsedSeal`. `PromoteUnitCommand {Unit, Option, SealSlot}`.
- **Promotion seals**: `ItemKind::PromotionSeal` (`Amount` = lowest unit level it works at, 1..20; `Uses` charges). With `SealSlot` the unit only needs the seal's level instead of the option's `MinLevel`; one charge is spent (removed at 0). `UseItem` refuses seals. JSON item `"kind": "seal", "level": N`.
- **Hit / avoid bonuses per weapon**: `WeaponSpec::Avoid` (-100..100), `RankHit`, `RankAvoid` (0..50 each, per wielder rank in the weapon's type). `CalculateHitRate` = clamp(Hit + RankHit*attackerRank + 2*Skill - (2*Speed + defenderWeapon.Avoid + RankAvoid*defenderRank), 0, 100); the defender's weapon counts only while usable. JSON `avoid`, `rankHit`, `rankAvoid`.
- **Strict `passable`**: `JsonRead::ReadBoolField`; `MapJsonParser` uses it.
- JSON: class `"promotion": {...}` became `"promotions": [ {level, to, bonus}, ... ]` (max 4, distinct targets; `FClassSpec::Promotions`); each option resolves to a `PromotionSpec` named after the target class. Sample data now has a soldier with two options (knight, ranger), a `cleric` class, an enemy healer (id 103), a `hero_seal`, a `rapier` (avoid) in `Data/`.
- Tests: CTest 17/17 (`PromotionTest`, `InventoryTest`, `CombatTest`, `EnemyAiTest` +9 healer cases, `BattleSessionTest`); Automation 14/14.

### Verified fact (was "probably, unverified")
- With the old `TryGetBoolField`, `"passable": 1`, `0` and `"true"` were all accepted by `MapJsonParser` (the new tests failed against the old code); `null` was already rejected. Fixed.

### Decisions
- Z1: the healer threshold (75%) and the staff XP formula are placeholders. The AI heals before attacking even when armed.
- Z2: no AI positioning for safety: an unarmed healer can end up next to player units while following allies.
- Z3: the AI considers only staffs; an enemy healer that is also hurt does not heal itself (UseStaff forbids self-targeting).
- Z4: `Promotions` replaced `Promotion` (breaking change inside the Domain/tests only, no saved data yet).

### Still not done
- Player-side hint/UI queries (which cells a healer can reach, which options a unit can take now).
- Item use by player AI (n/a), stealing, enemy drops, gold rewards.

## Update 2: trade/drop, shops, caps, promotion, weapon ranks, staffs (2026-11-07)

### Added (Domain; all numbers are placeholders, all rules self-made)
- **Give / drop**: `Battle::GiveItem(giver, receiver, slot)` -> `ItemGiven` (receiver adjacent, same side, room in pack; ends the giver's action). `Battle::DropItem(unit, slot)` -> `ItemDropped` (free).
- **Staff**: `ItemKind::Staff` (`Amount` heal, `Range`, `Uses`). `Battle::UseStaff(healer, target, slot)` -> `StaffUsed`: ally other than itself, distance 1..Range, target missing Hp; ends the healer's action; spent staffs are removed. `UseItem` rejects staff slots.
- **Shops / gold**: `Shop {Position, Stock}` (Battle/Shop.h); `Battle::AddShop`, `Shops()`, `Gold()`, `SetGold()`. `BuyItem(unit, entry)` -> `ItemBought`, `SellItem(unit, slot)` -> `ItemSold`: player unit standing on the shop cell; buying/selling is free (no action used); stock is unlimited; sell price = `Price / 2` (items with Price 0, or Price 1, cannot be sold). `ItemSlot::Price` (0 = not for sale); `AddShop` requires every stock item to be valid with Price > 0.
- **Stat caps**: `BattleUnit::Caps` (`StatCaps`, 0 = none; Hp is compared with MaxHp). `AddExperience` always rolls for every stat (so the random sequence does not depend on caps) and ignores a success at the cap.
- **Promotion**: `BattleUnit::Promotion` (optional `PromotionSpec {MinLevel, Bonus (StatBonus incl. Movement), Growth, Caps}`). `Battle::PromoteUnit(unit)` -> `UnitPromoted`: needs Level >= MinLevel; adds bonuses, Level 1, Experience 0, replaces Growth/Caps, consumes the promotion (once), ends the action.
- **Weapon ranks**: `WeaponSpec::Type` (0 = untyped, 1..8) and `RequiredRank` (0..3); `BattleUnit::WeaponPoints[type]` (training). `RankThreshold` = {0, 20, 50, 90} points, `WeaponRankOf`, `CanWield`, `CanUseWeapon`. A weapon above the unit's rank is not usable (`HasUsableWeapon` false) and cannot be equipped. Every strike of a **player** unit with a typed weapon adds 1 point (hit or miss); `Strike::WeaponRankUp` reports a rank change. Enemies' ranks are static.
- Validation is in `UnitValidation.h` (`IsPromotionValid`, caps, points, item slots incl. staffs/prices) and `IsWeaponConsistent` (type/rank ranges).

### Added (Application / Infrastructure)
- Commands + events: `GiveItemCommand`/`ItemGiven`, `DropItemCommand`/`ItemDropped`, `UseStaffCommand`/`StaffUsed`, `BuyItemCommand`/`ItemBought`, `SellItemCommand`/`ItemSold`, `PromoteUnitCommand`/`UnitPromoted`.
- `BattleSetup`: `Gold`, `Shops`; new error `InvalidShop` (negative gold or a bad shop), checked after units.
- AI: the spare-weapon swap now uses `CanUseWeapon`, so it skips spares the unit lacks the rank for.
- JSON: `Weapons.json` gains `types`, per-weapon `type` / `rank` / `price` (`FWeaponCatalog {Weapons, Types, Prices}` is now the parse result); `Items.json` gains `staff` (`range`) and `price`; `Classes.json` gains `caps` and `promotion {level, to, bonus}` (the target class supplies the new growth and caps; resolved when the unit is built, errors as `UnknownClass`); battle files gain `gold`, `shops [{x,y,stock}]` and per-unit `weaponRanks {type: rank}` (stored as the threshold points; unknown type -> `UnknownWeaponType`).
- Tests: `InventoryTest` (11), `PromotionTest` (11), `ItemTest`/`EnemyAiTest`/`BattleSessionTest`/`BattleSetupTest` extended (CTest 17/17); `Commander3D.Json.*` extended; Automation 14/14.

### Decisions
- Y1: trading is one-way "give" (no swap): two gives need two actions. Dropping is free; an equipped weapon cannot be given, dropped or sold (only pack items).
- Y2: gold lives on the `Battle` (the army's), not on units; no gold from kills/chests yet. Shops are map cells (not NPCs/buildings with interaction ranges).
- Y3: shopping is free so a unit can shop and still act; it requires the unit to be actionable (not already acted) and a player unit.
- Y4: ranks are plain thresholds on training points; no hit/avoid bonus per rank.
- Y5: a promotion happens once; there is no promotion item (level-gated only) and no choice of target class.
- Y6: weapon points are stored per unit (index = type id), so type ids are positions in the weapon file's `types` list: reordering that list changes the meaning of saved training (matters once saves exist, Phase 8).
- Y7: a test fixture without enemies ended the battle at the first `EndPhase` again (the wiring test); fixed in the test, not the code (see the earlier note on rout).

### Still not done
- AI does not use potions, staffs or repair kits, and does not weigh weapon wear. Healers are not special in the AI.
- Staff use gives no experience; no gold rewards; no item drops from enemies; no stealing/locked doors.
- Promotion target choice, promotion items, class-specific weapon usability beyond ranks.
- `MapJsonParser`'s `"passable"` still uses `TryGetBoolField` (probably accepts numbers).

## Update: the rest of Phase 7 (2026-11-07)

### Added
- **Experience / levels** (`Battle/Progression.h`): `MaxLevel` 20, `ExperiencePerLevel` 100, `CalculateExperience`, `AddExperience`. Events `ExperienceGained`, `LevelUp`; `AttackResolved` now has `Experience` and `LevelUps`. `BattleUnit`: `Level` (1), `Experience` (0..99), `Growth` (`GrowthRates`, percent per stat).
- **Items** (`BattleUnit.h`): `ItemKind {Weapon, Potion, RepairKit}`, `ItemSlot`, `MaxItemSlots` = 5, `BattleUnit::Items`. `Battle::EquipWeapon(unit, slot)` -> `WeaponEquipped`; `Battle::UseItem(unit, slot)` -> `ItemUsed {Kind, HpHealed, WeaponRepaired}`.
- **Validation** moved out of `AddUnit` into `Battle/UnitValidation.h` (`IsUnitDataValid`, `IsItemSlotValid`) and `IsWeaponConsistent` (Combat.h): level 1..20, exp 0..99, growth 0..100, <= 5 slots, slot data sane.
- **Application**: `EquipWeaponCommand`, `UseItemCommand`; events `WeaponEquipped`, `ItemUsed` added to `BattleEvent`. **Enemy AI**: a unit with no usable weapon equips the first usable spare weapon (`EquipWeaponCommand`) before anything else.
- **Infrastructure**: `FClassJsonParser` (`Classes.json`), `FItemJsonParser` (`Items.json`), `FBattleDataTables {Weapons, Items, Classes}`; `FBattleJsonParser::Parse(text, tables)` reads `class`, `level`, `exp`, `items`. `FBattleDataLoader` now also reads `Items.json` and `Classes.json` (new errors `InvalidItems`, `InvalidClasses`). Sample data: `Classes.json`, `Items.json`, `test_battle.json` updated.
- Tests: `ProgressionTest` (10), `ItemTest` (13), `BattleSessionTest` +1, `EnemyAiTest` +4 (CTest 15/15); `Commander3D.Json.*` extended (new `ParsesClassesAndItems`); Automation 14/14.

### Rules (own formulas, not copied from any title)
- XP per fight = 10 + 3 per level the opponent is above you + 20 for a kill; one level per 100. Only surviving player units that took part get it: the attacker, and the target if it struck back. Enemies never gain XP or levels.
- Level-up: one roll per stat (Hp, Strength, Skill, Speed, Defense, in that order) after the fight's strike rolls; attacker first, then target. Hp gain raises MaxHp and Hp.
- `EquipWeapon` is free (no action used), allowed before/after moving, swaps with the equipped weapon (an empty hand frees the slot). `UseItem` ends the action (sets HasMoved and HasActed). Potion refused at full Hp; repair kit refused for unbreakable or full-durability weapons. Emptied items are removed.

### Decisions
- X1: classes are data only: movement + growth, copied onto the unit at parse time (Domain has no class table), same pattern as weapons. An explicit `movement` overrides the class; without a class it is required.
- X2: enemy units keep `Level` for the XP formula (opponent level) but do not grow.
- X3: spare weapons come from `Weapons.json` ids; consumables from `Items.json` ids (weapon ids are looked up first; ids should not collide).
- X4: found by writing the strict-string tests: `FJsonObject::TryGetStringField` converts numbers to strings (like `TryGetBool`), so `ReadStringField` now checks `EJson::String`. This also tightens the weapon/map/battle parsers (e.g. `"map": 1` is now an error).
- X5: units cannot yet gain items/XP between battles (no persistence): Roster/Campaign context, Phase 8.

### Not done
- AI does not use potions or repair kits, and does not weigh weapon wear (e.g. saving durability for the boss). Only the broken-weapon swap is implemented.
- Item trading/dropping, shops, stat caps per class, promotion, weapon ranks, healing staves.
- `MapJsonParser`'s `"passable"` still uses `TryGetBoolField` (probably accepts numbers); unchanged and untested.
