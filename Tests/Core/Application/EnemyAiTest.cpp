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
