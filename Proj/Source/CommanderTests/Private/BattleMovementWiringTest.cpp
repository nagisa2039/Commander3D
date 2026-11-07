// Checks that Battle / MovementRange compile and work under UE's toolchain settings. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"

#include <vector>

#include "Battle/Battle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderBattleMovementWiringTest,
	"Commander3D.Wiring.BattleMovementIsUsable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderBattleMovementWiringTest::RunTest(const FString& Parameters)
{
	using namespace commander::domain;

	const std::vector<TerrainSpec> Table = {{1, true}, {0, false}};
	const std::vector<TerrainId> Cells = {0, 0, 0, 0, 1, 0};
	const auto Map = BattleMap::Create(*Grid::Create(3, 2), Table, Cells);
	TestTrue(TEXT("BattleMap created"), Map.has_value());
	if (!Map.has_value())
	{
		return false;
	}

	Battle B(*Map);
	TestTrue(TEXT("Unit added"), B.AddUnit(BattleUnit{UnitId{1}, Faction::Player, GridCoord{0, 0}, 3}));
	TestTrue(TEXT("Enemy added"), B.AddUnit(BattleUnit{UnitId{2}, Faction::Enemy, GridCoord{0, 1}, 1}));
	const auto Event = B.MoveUnit(UnitId{1}, GridCoord{2, 0});
	TestTrue(TEXT("Move succeeds"), Event.has_value());
	TestFalse(TEXT("Second move in the same phase rejected"), B.MoveUnit(UnitId{1}, GridCoord{1, 0}).has_value());
	TestTrue(TEXT("Wait succeeds"), B.WaitUnit(UnitId{1}).has_value());
	const PhaseChanged Enemy = B.EndPhase();
	TestTrue(TEXT("Enemy phase follows"), Enemy.Side == Faction::Enemy && Enemy.Turn == 1);
	const PhaseChanged Player = B.EndPhase();
	TestTrue(TEXT("Next turn starts with player"), Player.Side == Faction::Player && Player.Turn == 2);
	TestFalse(TEXT("Move onto wall rejected"), B.MoveUnit(UnitId{1}, GridCoord{1, 1}).has_value());

	// Combat: a unit with a sword hits an adjacent enemy, which is defeated.
	struct FZeroRandom : IRandomSource
	{
		int32 NextPercent() override { return 0; }
	};
	BattleUnit Attacker{UnitId{10}, Faction::Player, GridCoord{2, 0}, 1};
	Attacker.Strength = 5;
	Attacker.Weapon = WeaponSpec{3, 100, 1, 1};
	BattleUnit Target{UnitId{11}, Faction::Enemy, GridCoord{2, 1}, 1};
	Target.Hp = 4;
	Battle C(*Map);
	TestTrue(TEXT("Attacker added"), C.AddUnit(Attacker));
	TestTrue(TEXT("Target added"), C.AddUnit(Target));
	FZeroRandom Rng;
	const auto Attack = C.AttackUnit(UnitId{10}, UnitId{11}, Rng);
	TestTrue(TEXT("Attack resolved"), Attack.has_value());
	if (Attack.has_value())
	{
		TestTrue(TEXT("Target defeated"), Attack->Defeated.size() == 1 && C.FindUnit(UnitId{11}) == nullptr);
		TestTrue(TEXT("Last enemy defeated wins"), Attack->Outcome == BattleOutcome::PlayerWon && C.Outcome() == BattleOutcome::PlayerWon);
	}

	// Victory conditions: turn limit and boss.
	VictoryConditions Limited;
	Limited.TurnLimit = 1;
	Battle D(*Map, Limited);
	D.AddUnit(BattleUnit{UnitId{20}, Faction::Player, GridCoord{0, 0}, 1});
	D.AddUnit(BattleUnit{UnitId{21}, Faction::Enemy, GridCoord{2, 1}, 1});
	D.EndPhase();
	TestTrue(TEXT("Exceeding the turn limit loses"), D.EndPhase().Outcome == BattleOutcome::PlayerLost);

	const std::vector<BattleUnit> WithBoss = {BattleUnit{UnitId{30}, Faction::Player, GridCoord{0, 0}, 1}};
	VictoryConditions BossOnly;
	BossOnly.RoutEnemies = false;
	BossOnly.DefeatBoss = true;
	TestTrue(TEXT("No boss left wins"), EvaluateOutcome(WithBoss, BossOnly, 1) == BattleOutcome::PlayerWon);
	return true;
}
