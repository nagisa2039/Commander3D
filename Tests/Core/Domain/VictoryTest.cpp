#include <vector>

#include "Battle/Battle.h"
#include "TestSupport.h"

using namespace commander::domain;

class AlwaysHit : public IRandomSource
{
public:
	std::int32_t NextPercent() override { return 0; }
};

static BattleUnit Fighter(std::uint32_t Id, Faction Side, std::int32_t X, std::int32_t Hp)
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, 3};
	U.Hp = Hp;
	U.MaxHp = Hp;
	U.Strength = 5;
	U.Weapon = WeaponSpec{3, 100, 1, 1}; // 8 damage per hit
	return U;
}

static Battle MakeBattle(const std::vector<BattleUnit>& Units, VictoryConditions Conditions = {})
{
	const auto Map = BattleMap::Create(*Grid::Create(6, 1), {{1, true}}, {0, 0, 0, 0, 0, 0});
	Battle B(*Map, Conditions);
	for (const BattleUnit& U : Units)
	{
		B.AddUnit(U);
	}
	return B;
}

static void EvaluateOutcomeRules()
{
	const BattleUnit P = Fighter(1, Faction::Player, 0, 5);
	const BattleUnit E = Fighter(2, Faction::Enemy, 1, 5);
	CHECK(EvaluateOutcome({P, E}) == BattleOutcome::Ongoing);
	CHECK(EvaluateOutcome({P}) == BattleOutcome::PlayerWon);
	CHECK(EvaluateOutcome({E}) == BattleOutcome::PlayerLost);
	CHECK(EvaluateOutcome({}) == BattleOutcome::PlayerLost);
}

static void StartsOngoing()
{
	const Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 5), Fighter(2, Faction::Enemy, 1, 5)});
	CHECK(B.Outcome() == BattleOutcome::Ongoing);
}

static void DefeatingTheLastEnemyWins()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 1, 5)});
	AlwaysHit Rng;
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Outcome == BattleOutcome::PlayerWon);
	CHECK(B.Outcome() == BattleOutcome::PlayerWon);
}

static void NotWonWhileEnemiesRemain()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 1, 5), Fighter(3, Faction::Enemy, 4, 5)});
	AlwaysHit Rng;
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Outcome == BattleOutcome::Ongoing);
	CHECK(B.Outcome() == BattleOutcome::Ongoing);
}

static void LosingTheLastPlayerUnitLoses()
{
	// The only player unit attacks a strong enemy and dies to the counter.
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 5), Fighter(2, Faction::Enemy, 1, 50)});
	AlwaysHit Rng;
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Outcome == BattleOutcome::PlayerLost);
	CHECK(B.Outcome() == BattleOutcome::PlayerLost);
}

static void NoCommandsAfterTheBattleEnds()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 1, 5), Fighter(3, Faction::Player, 3, 20)});
	AlwaysHit Rng;
	B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(B.Outcome() == BattleOutcome::PlayerWon);
	CHECK(!B.MoveUnit(UnitId{3}, {4, 0}).has_value());
	CHECK(!B.WaitUnit(UnitId{3}).has_value());
	CHECK(!B.AttackUnit(UnitId{3}, UnitId{1}, Rng).has_value());
	const Faction Before = B.CurrentFaction();
	const PhaseChanged E = B.EndPhase();
	CHECK(B.CurrentFaction() == Before);
	CHECK(E.Side == Before && E.Turn == B.TurnNumber());
}

static BattleUnit Boss(std::uint32_t Id, std::int32_t X, std::int32_t Hp)
{
	BattleUnit U = Fighter(Id, Faction::Enemy, X, Hp);
	U.IsBoss = true;
	return U;
}

static void DefeatingTheBossWinsEvenIfOtherEnemiesRemain()
{
	VictoryConditions C;
	C.RoutEnemies = false;
	C.DefeatBoss = true;
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Boss(2, 1, 5), Fighter(3, Faction::Enemy, 4, 5)}, C);
	AlwaysHit Rng;
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Outcome == BattleOutcome::PlayerWon);
	CHECK(B.FindUnit(UnitId{3}) != nullptr);
}

static void DefeatingOtherEnemiesDoesNotWinWhileBossLives()
{
	VictoryConditions C;
	C.RoutEnemies = false;
	C.DefeatBoss = true;
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 1, 5), Boss(3, 4, 5)}, C);
	AlwaysHit Rng;
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Outcome == BattleOutcome::Ongoing);
}

static void BossRuleOffMeansBossIsOrdinary()
{
	// DefeatBoss disabled: killing the boss does not end the battle while another enemy remains.
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Boss(2, 1, 5), Fighter(3, Faction::Enemy, 4, 5)});
	AlwaysHit Rng;
	CHECK(B.AttackUnit(UnitId{1}, UnitId{2}, Rng)->Outcome == BattleOutcome::Ongoing);
}

static void RoutStillWinsWhenBossRuleIsAlsoOn()
{
	VictoryConditions C;
	C.DefeatBoss = true;
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 1, 5)}, C);
	AlwaysHit Rng;
	CHECK(B.AttackUnit(UnitId{1}, UnitId{2}, Rng)->Outcome == BattleOutcome::PlayerWon);
}

static void ExceedingTheTurnLimitLoses()
{
	VictoryConditions C;
	C.TurnLimit = 2;
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 5, 20)}, C);
	CHECK(B.EndPhase().Outcome == BattleOutcome::Ongoing); // enemy phase, turn 1
	CHECK(B.EndPhase().Outcome == BattleOutcome::Ongoing); // player phase, turn 2 (last allowed turn)
	CHECK(B.EndPhase().Outcome == BattleOutcome::Ongoing); // enemy phase, turn 2
	const PhaseChanged E = B.EndPhase();                    // would start turn 3
	CHECK(E.Turn == 3);
	CHECK(E.Outcome == BattleOutcome::PlayerLost);
	CHECK(B.Outcome() == BattleOutcome::PlayerLost);
	CHECK(!B.WaitUnit(UnitId{1}).has_value());
}

static void WinningOnTheLastTurnBeatsTheLimit()
{
	VictoryConditions C;
	C.TurnLimit = 1;
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 1, 5)}, C);
	AlwaysHit Rng;
	CHECK(B.AttackUnit(UnitId{1}, UnitId{2}, Rng)->Outcome == BattleOutcome::PlayerWon);
	CHECK(B.EndPhase().Outcome == BattleOutcome::PlayerWon);
}

static void NoTurnLimitByDefault()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 5, 20)});
	for (int I = 0; I < 40; ++I)
	{
		B.EndPhase();
	}
	CHECK(B.Outcome() == BattleOutcome::Ongoing);
	CHECK(B.TurnNumber() == 21);
}

int main()
{
	DefeatingTheBossWinsEvenIfOtherEnemiesRemain();
	DefeatingOtherEnemiesDoesNotWinWhileBossLives();
	BossRuleOffMeansBossIsOrdinary();
	RoutStillWinsWhenBossRuleIsAlsoOn();
	ExceedingTheTurnLimitLoses();
	WinningOnTheLastTurnBeatsTheLimit();
	NoTurnLimitByDefault();
	EvaluateOutcomeRules();
	StartsOngoing();
	DefeatingTheLastEnemyWins();
	NotWonWhileEnemiesRemain();
	LosingTheLastPlayerUnitLoses();
	NoCommandsAfterTheBattleEnds();
	return TEST_MAIN_RESULT();
}
