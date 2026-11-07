#include <variant>
#include <vector>

#include "Battle/EnemyAi.h"
#include "TestSupport.h"

using namespace commander::application;
using namespace commander::domain;

class AlwaysHit : public IRandomSource
{
public:
	std::int32_t NextPercent() override { return 0; }
};

// 8 damage per hit against Defense 0; sword range 1.
static BattleUnit Fighter(std::uint32_t Id, Faction Side, std::int32_t X, std::int32_t Hp, std::int32_t Movement = 3,
	std::int32_t Defense = 0)
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, Movement};
	U.Hp = U.MaxHp = Hp;
	U.Strength = 5;
	U.Defense = Defense;
	U.Weapon = WeaponSpec{3, 100, 1, 1};
	return U;
}

static Battle MakeBattle(const std::vector<BattleUnit>& Units)
{
	const auto Map = BattleMap::Create(*Grid::Create(6, 1), {{1, true}}, {0, 0, 0, 0, 0, 0});
	Battle B(*Map);
	for (const BattleUnit& U : Units)
	{
		B.AddUnit(U);
	}
	return B;
}

// A battle already in the enemy phase.
static Battle EnemyPhase(const std::vector<BattleUnit>& Units)
{
	Battle B = MakeBattle(Units);
	B.EndPhase();
	return B;
}

template <typename T>
static const T* As(const std::optional<BattleCommand>& C)
{
	return C.has_value() ? std::get_if<T>(&*C) : nullptr;
}

static void NoCommandOutsideTheEnemyPhaseOrAfterTheBattleEnds()
{
	const Battle PlayerPhase = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 1, 20)});
	CHECK(!DecideNext(PlayerPhase).has_value());

	// The player defeats the last enemy: the battle is over, so the AI has nothing to do.
	Battle Won = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 1, 5), Fighter(8, Faction::Enemy, 5, 5)});
	AlwaysHit Rng;
	Won.AttackUnit(UnitId{1}, UnitId{9}, Rng);
	CHECK(Won.Outcome() == BattleOutcome::Ongoing); // unit 8 is still alive
	Won.EndPhase();
	CHECK(DecideNext(Won).has_value());

	Battle Over = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 1, 5)});
	Over.AttackUnit(UnitId{1}, UnitId{9}, Rng);
	CHECK(Over.Outcome() == BattleOutcome::PlayerWon);
	CHECK(!DecideNext(Over).has_value());
}

static void AttacksAnAdjacentTargetWithoutMoving()
{
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 1, 20)});
	const auto* Attack = As<AttackUnitCommand>(DecideNext(B));
	CHECK(Attack != nullptr);
	CHECK(Attack != nullptr && Attack->Attacker == UnitId{9} && Attack->Target == UnitId{1});
}

static void MovesIntoRangeBeforeAttacking()
{
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 3, 20)});
	const auto* Move = As<MoveUnitCommand>(DecideNext(B));
	CHECK(Move != nullptr);
	CHECK(Move != nullptr && Move->Unit == UnitId{9} && Move->Destination == (GridCoord{1, 0}));
}

static void PrefersAKillOverMoreDamage()
{
	// Both targets are adjacent to the enemy at x=2 and take 8 damage; only unit 2 dies.
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 1, 20), Fighter(2, Faction::Player, 3, 5), Fighter(9, Faction::Enemy, 2, 20)});
	const auto* Attack = As<AttackUnitCommand>(DecideNext(B));
	CHECK(Attack != nullptr && Attack->Target == UnitId{2});
}

static void PrefersMoreDamageWhenNoKill()
{
	// Unit 1 (armoured, lower id) takes 3, unit 2 takes 8.
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 1, 20, 3, 5), Fighter(2, Faction::Player, 3, 20), Fighter(9, Faction::Enemy, 2, 20)});
	const auto* Attack = As<AttackUnitCommand>(DecideNext(B));
	CHECK(Attack != nullptr && Attack->Target == UnitId{2});
}

static void ApproachesWhenNothingIsInReach()
{
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 5, 20, 2)});
	const auto* Move = As<MoveUnitCommand>(DecideNext(B));
	CHECK(Move != nullptr && Move->Destination == (GridCoord{3, 0}));
}

static BattleUnit At(std::uint32_t Id, Faction Side, std::int32_t X, std::int32_t Y, std::int32_t Movement)
{
	BattleUnit U = Fighter(Id, Side, X, 20, Movement);
	U.Position = {X, Y};
	return U;
}

// 5x3, W = wall:
//   . . . . .
//   W W W . .
//   . . . . .
static Battle WalledBattle(const std::vector<BattleUnit>& Units)
{
	const auto Map = BattleMap::Create(*Grid::Create(5, 3), {{1, true}, {0, false}}, {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0});
	Battle B(*Map);
	for (const BattleUnit& U : Units)
	{
		B.AddUnit(U);
	}
	B.EndPhase();
	return B;
}

static void ApproachFollowsThePathAroundAWall()
{
	// The player at (2,0) is 2 cells away as the crow flies but 4 by path. By Manhattan distance every
	// reachable cell is no closer than the start; by path, (3,1) (cost 2 away) is the best step.
	const Battle B = WalledBattle({At(1, Faction::Player, 2, 0, 3), At(9, Faction::Enemy, 2, 2, 2)});
	const auto* Move = As<MoveUnitCommand>(DecideNext(B));
	CHECK(Move != nullptr);
	CHECK(Move != nullptr && Move->Destination == (GridCoord{3, 1}));
}

static void ApproachPrefersTheShorterOfTwoRoutes()
{
	// Two ways around; the player is closer to the right-hand gap.
	const Battle B = WalledBattle({At(1, Faction::Player, 3, 0, 3), At(9, Faction::Enemy, 2, 2, 1)});
	const auto* Move = As<MoveUnitCommand>(DecideNext(B));
	CHECK(Move != nullptr && Move->Destination == (GridCoord{3, 2}));
}

static void ApproachChoosesTheReachablePlayerWhenAnotherIsWalledOff()
{
	// Unit 1 at (0,0) is sealed in by walls and is the nearest as the crow flies; unit 2 is reachable.
	// The enemy at (2,0) can only step to (3,0), which is 3 from unit 2 (the start is 4).
	const auto Map = BattleMap::Create(*Grid::Create(5, 3), {{1, true}, {0, false}},
		{0, 1, 0, 0, 0,
		 1, 1, 1, 0, 0,
		 0, 0, 0, 0, 0});
	Battle B(*Map);
	B.AddUnit(At(1, Faction::Player, 0, 0, 3));
	B.AddUnit(At(2, Faction::Player, 4, 2, 3));
	B.AddUnit(At(9, Faction::Enemy, 2, 0, 1));
	B.EndPhase();
	const auto* Move = As<MoveUnitCommand>(DecideNext(B));
	CHECK(Move != nullptr);
	CHECK(Move != nullptr && Move->Destination == (GridCoord{3, 0}));
}

static void WaitsWhenNoPlayerUnitIsReachableByPath()
{
	const auto Map = BattleMap::Create(*Grid::Create(3, 1), {{1, true}, {0, false}}, {0, 1, 0});
	Battle B(*Map);
	B.AddUnit(Fighter(1, Faction::Player, 0, 20));
	B.AddUnit(Fighter(9, Faction::Enemy, 2, 20));
	B.EndPhase();
	CHECK(As<WaitUnitCommand>(DecideNext(B)) != nullptr);
}

static ItemSlot SpareSword(std::int32_t Durability = 0, std::int32_t Uses = 0)
{
	ItemSlot S;
	S.Kind = ItemKind::Weapon;
	S.Weapon = WeaponSpec{3, 100, 1, 1, Durability};
	S.Uses = Uses;
	return S;
}

// An enemy next to the player whose equipped lance has broken.
static BattleUnit BrokenLanceEnemy(std::vector<ItemSlot> Spares)
{
	BattleUnit U = Fighter(9, Faction::Enemy, 1, 20);
	U.Weapon = WeaponSpec{3, 100, 1, 1, 10};
	U.WeaponUses = 0;
	U.Items = std::move(Spares);
	return U;
}

static void EquipsASpareWeaponWhenTheEquippedOneIsBroken()
{
	AlwaysHit Rng;
	BattleSession S(EnemyPhase({Fighter(1, Faction::Player, 0, 20), BrokenLanceEnemy({SpareSword()})}), Rng);
	const auto* Equip = As<EquipWeaponCommand>(DecideNext(S.GetBattle()));
	CHECK(Equip != nullptr && Equip->Unit == UnitId{9} && Equip->Slot == 0);
	CHECK(S.Execute(*DecideNext(S.GetBattle())).IsOk());
	CHECK(As<AttackUnitCommand>(DecideNext(S.GetBattle())) != nullptr); // armed again, so it attacks
}

static void SkipsBrokenSparesAndPicksTheFirstUsableOne()
{
	// Slot 0 is a broken spare, slot 1 is usable.
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 0, 20), BrokenLanceEnemy({SpareSword(5, 0), SpareSword(5, 3)})});
	const auto* Equip = As<EquipWeaponCommand>(DecideNext(B));
	CHECK(Equip != nullptr && Equip->Slot == 1);
}

static void SkipsSparesThatNeedAHigherRank()
{
	ItemSlot Heavy = SpareSword();
	Heavy.Weapon.Type = 1;
	Heavy.Weapon.RequiredRank = 2; // the enemy has no training in type 1
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 0, 20), BrokenLanceEnemy({Heavy, SpareSword()})});
	const auto* Equip = As<EquipWeaponCommand>(DecideNext(B));
	CHECK(Equip != nullptr && Equip->Slot == 1);
}

static void DoesNotEquipWhenThereIsNoUsableSpare()
{
	const Battle NoSpare = EnemyPhase({Fighter(1, Faction::Player, 0, 20), BrokenLanceEnemy({})});
	CHECK(As<EquipWeaponCommand>(DecideNext(NoSpare)) == nullptr);
	const Battle OnlyBroken = EnemyPhase({Fighter(1, Faction::Player, 0, 20), BrokenLanceEnemy({SpareSword(5, 0)})});
	CHECK(As<EquipWeaponCommand>(DecideNext(OnlyBroken)) == nullptr);
}

static void RunEnemyPhaseIncludesTheEquipStep()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 30), BrokenLanceEnemy({SpareSword()})}), Rng);
	S.Execute(EndPhaseCommand{});
	const std::vector<BattleEvent> Events = RunEnemyPhase(S);
	CHECK(Events.size() == 3); // equip, attack, end phase
	if (Events.size() == 3)
	{
		CHECK(std::holds_alternative<WeaponEquipped>(Events[0]));
		CHECK(std::holds_alternative<AttackResolved>(Events[1]));
	}
}

// ---- healers ----

static ItemSlot StaffItem(std::int32_t Range = 2, std::int32_t Amount = 8, std::int32_t Uses = 3)
{
	ItemSlot S;
	S.Kind = ItemKind::Staff;
	S.Range = Range;
	S.Amount = Amount;
	S.Uses = Uses;
	return S;
}

// An enemy-side healer: no weapon, one staff.
static BattleUnit Healer(std::uint32_t Id, std::int32_t X, std::int32_t Range = 2, std::int32_t Movement = 3)
{
	BattleUnit U = Fighter(Id, Faction::Enemy, X, 20, Movement);
	U.Weapon = WeaponSpec{};
	U.Items = {StaffItem(Range)};
	return U;
}

static BattleUnit Wounded(std::uint32_t Id, std::int32_t X, std::int32_t Hp)
{
	BattleUnit U = Fighter(Id, Faction::Enemy, X, 20);
	U.Hp = Hp;
	return U;
}

static void AHealerHealsAWoundedAllyInRange()
{
	const Battle B = EnemyPhase({Healer(8, 4), Wounded(9, 5, 6), Fighter(1, Faction::Player, 0, 20)});
	const auto* Heal = As<UseStaffCommand>(DecideNext(B));
	CHECK(Heal != nullptr && Heal->Healer == UnitId{8} && Heal->Target == UnitId{9} && Heal->Slot == 0);
}

static void AHealerMovesIntoRangeThenHeals()
{
	AlwaysHit Rng;
	// Staff range 1, movement 2: the only cell next to the ally (x=5) it can reach is x=4.
	BattleSession S(EnemyPhase({Healer(8, 2, 1, 2), Wounded(9, 5, 6), Fighter(1, Faction::Player, 0, 20)}), Rng);
	const auto* Move = As<MoveUnitCommand>(DecideNext(S.GetBattle()));
	CHECK(Move != nullptr && Move->Unit == UnitId{8} && Move->Destination == (GridCoord{4, 0}));
	CHECK(S.Execute(*DecideNext(S.GetBattle())).IsOk());
	const auto* Heal = As<UseStaffCommand>(DecideNext(S.GetBattle()));
	CHECK(Heal != nullptr && Heal->Target == UnitId{9});
}

static void AHealerPicksTheMostWoundedAlly()
{
	// 10/20 (50%) at x=3 and 5/20 (25%) at x=5; the healer at x=4 reaches both.
	const Battle B = EnemyPhase({Healer(8, 4), Wounded(7, 3, 10), Wounded(9, 5, 5), Fighter(1, Faction::Player, 0, 20)});
	const auto* Heal = As<UseStaffCommand>(DecideNext(B));
	CHECK(Heal != nullptr && Heal->Target == UnitId{9});
}

static void AHealerLeavesLightlyHurtAlliesAlone()
{
	// 16/20 is 80%: above the 75% threshold.
	const Battle B = EnemyPhase({Healer(8, 4), Wounded(9, 5, 16), Fighter(1, Faction::Player, 0, 20)});
	CHECK(As<UseStaffCommand>(DecideNext(B)) == nullptr);
	const Battle AtThreshold = EnemyPhase({Healer(8, 4), Wounded(9, 5, 15), Fighter(1, Faction::Player, 0, 20)});
	CHECK(As<UseStaffCommand>(DecideNext(AtThreshold)) != nullptr); // 15/20 = 75% is healed
}

static void AnUnarmedHealerFollowsAWoundedAllyElseWaits()
{
	// Nothing to heal yet (80%), but the ally is hurt: the healer walks toward it (movement 2, so x=1 -> x=3).
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 0, 20), Healer(8, 1, 1, 2), Wounded(9, 5, 16)});
	const auto* Move = As<MoveUnitCommand>(DecideNext(B));
	CHECK(Move != nullptr && Move->Unit == UnitId{8} && Move->Destination == (GridCoord{3, 0})); // two steps toward the ally

	const Battle Healthy = EnemyPhase({Fighter(1, Faction::Player, 0, 20), Healer(8, 3), Fighter(9, Faction::Enemy, 5, 20)});
	CHECK(As<WaitUnitCommand>(DecideNext(Healthy)) != nullptr); // no one is hurt: it stays out of the fight
}

static void AnArmedHealerFightsWhenThereIsNoOneToHeal()
{
	BattleUnit Cleric = Healer(8, 1);
	Cleric.Weapon = WeaponSpec{3, 100, 1, 1};
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 0, 20), Cleric, Fighter(9, Faction::Enemy, 5, 20)});
	const auto* Attack = As<AttackUnitCommand>(DecideNext(B));
	CHECK(Attack != nullptr && Attack->Attacker == UnitId{8} && Attack->Target == UnitId{1});
}

static void AHealerHealsBeforeItFights()
{
	BattleUnit Cleric = Healer(8, 1);
	Cleric.Weapon = WeaponSpec{3, 100, 1, 1};
	const Battle B = EnemyPhase({Fighter(1, Faction::Player, 0, 20), Cleric, Wounded(9, 2, 4)});
	CHECK(As<UseStaffCommand>(DecideNext(B)) != nullptr);
}

static void EnemiesNeverUsePotionsOrRepairKits()
{
	ItemSlot Potion;
	Potion.Kind = ItemKind::Potion;
	Potion.Uses = 2;
	Potion.Amount = 10;
	ItemSlot Repair;
	Repair.Kind = ItemKind::RepairKit;
	Repair.Uses = 1;
	BattleUnit Brute = BrokenLanceEnemy({Potion, Repair});
	Brute.Hp = 3; // badly hurt, broken weapon, but it still does not drink or repair
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 30), Brute}), Rng);
	S.Execute(EndPhaseCommand{});
	for (const BattleEvent& E : RunEnemyPhase(S))
	{
		CHECK(!std::holds_alternative<ItemUsed>(E));
	}
	CHECK(S.GetBattle().FindUnit(UnitId{9}) == nullptr || S.GetBattle().FindUnit(UnitId{9})->Items.size() == 2);
}

static void RunEnemyPhasePlaysTheHeal()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 30), Healer(8, 4), Wounded(9, 5, 6)}), Rng);
	S.Execute(EndPhaseCommand{});
	const std::vector<BattleEvent> Events = RunEnemyPhase(S);
	bool bHealed = false;
	for (const BattleEvent& E : Events)
	{
		if (const auto* Used = std::get_if<StaffUsed>(&E))
		{
			bHealed = true;
			CHECK(Used->HpHealed == 8 && !Used->Experience.has_value()); // enemy healers earn nothing
		}
	}
	CHECK(bHealed);
	CHECK(S.GetBattle().FindUnit(UnitId{9}) != nullptr);
	CHECK(std::holds_alternative<PhaseChanged>(Events.back()));
}

static void WaitsWhenItCannotApproachThenEndsThePhase()
{
	AlwaysHit Rng;
	BattleSession S(EnemyPhase({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 5, 20, 0)}), Rng);
	const auto* Wait = As<WaitUnitCommand>(DecideNext(S.GetBattle()));
	CHECK(Wait != nullptr && Wait->Unit == UnitId{9});
	CHECK(S.Execute(*DecideNext(S.GetBattle())).IsOk());
	CHECK(As<EndPhaseCommand>(DecideNext(S.GetBattle())) != nullptr);
}

static void RunEnemyPhasePlaysMoveAttackAndEndsThePhase()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 2, 20)}), Rng);
	S.Execute(EndPhaseCommand{});
	const std::vector<BattleEvent> Events = RunEnemyPhase(S);
	CHECK(Events.size() == 3);
	if (Events.size() == 3)
	{
		CHECK(std::holds_alternative<UnitMoved>(Events[0]));
		CHECK(std::holds_alternative<AttackResolved>(Events[1]));
		const auto* Phase = std::get_if<PhaseChanged>(&Events[2]);
		CHECK(Phase != nullptr && Phase->Side == Faction::Player && Phase->Turn == 2);
	}
	CHECK(S.GetBattle().CurrentFaction() == Faction::Player);
	CHECK(S.GetBattle().FindUnit(UnitId{1})->Hp == 12);
	CHECK(S.GetBattle().FindUnit(UnitId{9})->Hp == 12); // countered
}

static void RunEnemyPhaseStopsWhenThePlayerIsWiped()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 8), Fighter(9, Faction::Enemy, 2, 20)}), Rng);
	S.Execute(EndPhaseCommand{});
	const std::vector<BattleEvent> Events = RunEnemyPhase(S);
	CHECK(Events.size() == 2); // move, attack; no EndPhase after the battle ended
	CHECK(S.GetBattle().Outcome() == BattleOutcome::PlayerLost);
}

static void SeveralEnemiesDoNotShareACell()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 40), Fighter(8, Faction::Enemy, 4, 20), Fighter(9, Faction::Enemy, 5, 20)}), Rng);
	S.Execute(EndPhaseCommand{});
	RunEnemyPhase(S);
	CHECK(S.GetBattle().FindUnit(UnitId{8})->Position == (GridCoord{1, 0}));
	CHECK(S.GetBattle().FindUnit(UnitId{9})->Position == (GridCoord{2, 0}));
}

static void RunEnemyPhaseDoesNothingDuringThePlayerPhase()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(9, Faction::Enemy, 2, 20)}), Rng);
	CHECK(RunEnemyPhase(S).empty());
	CHECK(S.GetBattle().CurrentFaction() == Faction::Player);
}

int main()
{
	NoCommandOutsideTheEnemyPhaseOrAfterTheBattleEnds();
	AttacksAnAdjacentTargetWithoutMoving();
	MovesIntoRangeBeforeAttacking();
	PrefersAKillOverMoreDamage();
	PrefersMoreDamageWhenNoKill();
	ApproachesWhenNothingIsInReach();
	EquipsASpareWeaponWhenTheEquippedOneIsBroken();
	SkipsBrokenSparesAndPicksTheFirstUsableOne();
	AHealerHealsAWoundedAllyInRange();
	AHealerMovesIntoRangeThenHeals();
	AHealerPicksTheMostWoundedAlly();
	AHealerLeavesLightlyHurtAlliesAlone();
	AnUnarmedHealerFollowsAWoundedAllyElseWaits();
	AnArmedHealerFightsWhenThereIsNoOneToHeal();
	AHealerHealsBeforeItFights();
	EnemiesNeverUsePotionsOrRepairKits();
	RunEnemyPhasePlaysTheHeal();
	SkipsSparesThatNeedAHigherRank();
	DoesNotEquipWhenThereIsNoUsableSpare();
	RunEnemyPhaseIncludesTheEquipStep();
	ApproachFollowsThePathAroundAWall();
	ApproachPrefersTheShorterOfTwoRoutes();
	ApproachChoosesTheReachablePlayerWhenAnotherIsWalledOff();
	WaitsWhenNoPlayerUnitIsReachableByPath();
	WaitsWhenItCannotApproachThenEndsThePhase();
	RunEnemyPhasePlaysMoveAttackAndEndsThePhase();
	RunEnemyPhaseStopsWhenThePlayerIsWiped();
	SeveralEnemiesDoNotShareACell();
	RunEnemyPhaseDoesNothingDuringThePlayerPhase();
	return TEST_MAIN_RESULT();
}
