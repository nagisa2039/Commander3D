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

	// Items and experience: a potion heals, a spare weapon is equipped, a kill grants experience.
	ItemSlot Potion;
	Potion.Kind = ItemKind::Potion;
	Potion.Uses = 1;
	Potion.Amount = 5;
	ItemSlot Spare;
	Spare.Kind = ItemKind::Weapon;
	Spare.Weapon = WeaponSpec{4, 100, 1, 1};
	BattleUnit Medic{UnitId{40}, Faction::Player, GridCoord{0, 0}, 1};
	Medic.Hp = 3;
	Medic.MaxHp = 10;
	Medic.Items = {Potion, Spare};
	Battle E(*Map);
	TestTrue(TEXT("Medic added"), E.AddUnit(Medic));
	E.AddUnit(BattleUnit{UnitId{41}, Faction::Enemy, GridCoord{2, 1}, 1});
	const auto Healed = E.UseItem(UnitId{40}, 0);
	TestTrue(TEXT("Potion heals"), Healed.has_value() && Healed->HpHealed == 5 && E.FindUnit(UnitId{40})->Hp == 8);
	E.EndPhase();
	E.EndPhase();
	TestTrue(TEXT("Spare weapon equipped"), E.EquipWeapon(UnitId{40}, 0).has_value() && E.FindUnit(UnitId{40})->Weapon.Might == 4);

	// Staff, shop, promotion and weapon ranks.
	ItemSlot Staff;
	Staff.Kind = ItemKind::Staff;
	Staff.Uses = 1;
	Staff.Amount = 4;
	Staff.Range = 2;
	BattleUnit Cleric{UnitId{50}, Faction::Player, GridCoord{0, 0}, 1};
	Cleric.Items = {Staff};
	Cleric.Level = 3;
	Cleric.Promotions = {PromotionSpec{"bishop", 3, StatBonus{1, 0, 0, 0, 0, 1}, GrowthRates{}, StatCaps{}}};
	BattleUnit Hurt{UnitId{51}, Faction::Player, GridCoord{1, 0}, 1};
	Hurt.Hp = 2;
	Hurt.MaxHp = 9;
	Battle F(*Map);
	TestTrue(TEXT("Cleric and wounded ally added"), F.AddUnit(Cleric) && F.AddUnit(Hurt));
	TestTrue(TEXT("An enemy keeps the battle going"), F.AddUnit(BattleUnit{UnitId{52}, Faction::Enemy, GridCoord{2, 1}, 1}));
	FZeroRandom StaffRng;
	const auto StaffHeal = F.UseStaff(UnitId{50}, UnitId{51}, 0, StaffRng);
	TestTrue(TEXT("Staff heals and earns experience"),
		StaffHeal.has_value() && F.FindUnit(UnitId{51})->Hp == 6 && StaffHeal->Experience.has_value() && StaffHeal->Experience->Amount == 10);
	F.EndPhase();
	F.EndPhase();
	const auto Promoted = F.PromoteUnit(UnitId{50}, 0);
	TestTrue(TEXT("Promotion works"), Promoted.has_value() && Promoted->Name == "bishop" && F.FindUnit(UnitId{50})->Level == 1);
	Shop Store;
	Store.Position = GridCoord{1, 0};
	ItemSlot ForSale;
	ForSale.Kind = ItemKind::RepairKit;
	ForSale.Uses = 1;
	ForSale.Price = 10;
	Store.Stock = {ForSale};
	TestTrue(TEXT("Shop added"), F.AddShop(Store) && F.SetGold(25));
	TestTrue(TEXT("Buying works"), F.BuyItem(UnitId{51}, 0).has_value() && F.Gold() == 15);
	TestTrue(TEXT("Selling works"), F.SellItem(UnitId{51}, 0).has_value() && F.Gold() == 20);
	TestEqual(TEXT("Rank thresholds"), WeaponRankOf(RankThreshold[2]), 2);

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
