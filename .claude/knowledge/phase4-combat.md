# Phase 4: weapons / attack / damage / HP / counter / RNG / Events (Domain only)

## Added
- `BattleUnit`: `Hp` (default 1), `MaxHp`, `Strength`, `Skill`, `Speed`, `Defense`, `Weapon` (`WeaponSpec {Might, Hit, MinRange, MaxRange}`; MaxRange 0 = no weapon).
- `Battle/Combat.h`: `CalculateDamage`, `CalculateHitRate`, `WeaponReaches`.
- `Battle/RandomSource.h`: `IRandomSource::NextPercent()` (0..99).
- `Battle::AttackUnit(attacker, target, Rng)` -> `optional<AttackResolved {Strikes, Defeated}>`. `Strike {Attacker, Defender, Hit, Damage, DefenderHp}`.
- `AddUnit` rejects Hp <= 0.
- Tests: `Tests/Core/Domain/CombatTest.cpp` (11 cases, FixedRandom fake); wiring test extended.

## Formulas (original, not copied from any title)
- Damage = max(0, Strength + weapon Might - target Defense).
- Hit rate = clamp(weapon Hit + 2*Skill - 2*target Speed, 0, 100). Hit when roll < rate.

## Decisions
- C1: range is measured from the attacker's current position (attack after move allowed). Weapon range is Manhattan MinRange..MaxRange.
- C2: sequence = attacker strike, then counter if target alive and attacker within target's weapon range. Kill prevents counter.
- C3: dead units are removed from the unit list; the Event carries `Defeated` ids. The attacker's HasMoved/HasActed are set (even if it died; it is removed).
- C4: RNG consumed once per strike (also for misses); order = Strikes order. Tests rely on this.
- C5: Hp default is 1 so earlier movement tests keep working; real data must set Hp.
- C6: no follow-up (double) attacks, no crits, no weapon triangle, no terrain defense bonus, no weapon durability (Phase 7). Add only with a concrete rule.

## Verified (2026-11-07)
- CTest 8/8, purity check OK, `Commander3DEditor` build succeeded, Automation `Commander3D.Wiring` 4/4 Success (one run failed first due to my wiring test placing a unit on a wall; fixed in the test, not the code).

## Not done
- Application Commands / BattleSession, JSON unit/weapon data, victory conditions (Phase 5), attackable-target query for UI, Event playback.
