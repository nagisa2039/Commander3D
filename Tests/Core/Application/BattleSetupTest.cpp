#include <vector>

#include "Battle/BattleSetup.h"
#include "TestSupport.h"

using namespace commander::application;
using namespace commander::domain;

static BattleUnit Unit(std::uint32_t Id, Faction Side, std::int32_t X, bool IsBoss = false)
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, 3};
	U.IsBoss = IsBoss;
	return U;
}

static BattleSetup ValidSetup()
{
	BattleSetup S;
	S.Map.Width = 4;
	S.Map.Height = 1;
	S.Map.Terrains = {{1, true}, {0, false}};
	S.Map.Cells = {0, 0, 0, 1};
	S.Units = {Unit(1, Faction::Player, 0), Unit(2, Faction::Enemy, 2, true)};
	return S;
}

static void ValidSetupStartsOngoingOnTurnOneWithPlayerPhase()
{
	const auto R = StartBattle(ValidSetup());
	CHECK(R.IsOk());
	CHECK(R.Value().Units().size() == 2);
	CHECK(R.Value().Outcome() == BattleOutcome::Ongoing);
	CHECK(R.Value().TurnNumber() == 1);
	CHECK(R.Value().CurrentFaction() == Faction::Player);
}

static void ConditionsAreApplied()
{
	BattleSetup S = ValidSetup();
	S.Conditions.TurnLimit = 1;
	auto R = StartBattle(S);
	CHECK(R.IsOk());
	Battle B = R.Value();
	B.EndPhase();
	CHECK(B.EndPhase().Outcome == BattleOutcome::PlayerLost);
}

static void InvalidMapIsReported()
{
	BattleSetup S = ValidSetup();
	S.Map.Width = 0;
	CHECK(StartBattle(S).Error() == BattleSetupError::InvalidMap);
}

static void UnplaceableUnitsAreReported()
{
	BattleSetup S = ValidSetup();
	S.Units.push_back(Unit(3, Faction::Enemy, 3));     // wall
	CHECK(StartBattle(S).Error() == BattleSetupError::InvalidUnit);
	S = ValidSetup();
	S.Units.push_back(Unit(1, Faction::Enemy, 1));     // duplicate id
	CHECK(StartBattle(S).Error() == BattleSetupError::InvalidUnit);
	S = ValidSetup();
	S.Units.push_back(Unit(3, Faction::Enemy, 2));     // occupied
	CHECK(StartBattle(S).Error() == BattleSetupError::InvalidUnit);
}

static void BothSidesAreRequired()
{
	BattleSetup S = ValidSetup();
	S.Units = {Unit(2, Faction::Enemy, 2)};
	CHECK(StartBattle(S).Error() == BattleSetupError::NoPlayerUnit);
	S.Units = {Unit(1, Faction::Player, 0)};
	CHECK(StartBattle(S).Error() == BattleSetupError::NoEnemyUnit);
}

static void BossIsRequiredWhenTheBossRuleIsOn()
{
	BattleSetup S = ValidSetup();
	S.Conditions.DefeatBoss = true;
	S.Units = {Unit(1, Faction::Player, 0), Unit(2, Faction::Enemy, 2)};
	CHECK(StartBattle(S).Error() == BattleSetupError::NoBoss);
	S.Units[1].IsBoss = true;
	CHECK(StartBattle(S).IsOk());
	// A boss on the player side does not count.
	S.Units = {Unit(1, Faction::Player, 0, true), Unit(2, Faction::Enemy, 2)};
	CHECK(StartBattle(S).Error() == BattleSetupError::NoBoss);
}

static Shop ShopAt(std::int32_t X, std::int32_t Price)
{
	ItemSlot Potion;
	Potion.Kind = ItemKind::Potion;
	Potion.Uses = 1;
	Potion.Amount = 5;
	Potion.Price = Price;
	Shop S;
	S.Position = {X, 0};
	S.Stock = {Potion};
	return S;
}

static void GoldAndShopsAreApplied()
{
	BattleSetup S = ValidSetup();
	S.Gold = 250;
	S.Shops = {ShopAt(1, 30)};
	const auto R = StartBattle(S);
	CHECK(R.IsOk());
	CHECK(R.Value().Gold() == 250);
	CHECK(R.Value().Shops().size() == 1 && R.Value().Shops()[0].Stock[0].Price == 30);
}

static void InvalidShopsAndGoldAreReported()
{
	BattleSetup S = ValidSetup();
	S.Shops = {ShopAt(3, 30)};                    // (3,0) is a wall
	CHECK(StartBattle(S).Error() == BattleSetupError::InvalidShop);
	S.Shops = {ShopAt(1, 0)};                     // not for sale
	CHECK(StartBattle(S).Error() == BattleSetupError::InvalidShop);
	S.Shops = {ShopAt(1, 30), ShopAt(1, 30)};     // two shops on one cell
	CHECK(StartBattle(S).Error() == BattleSetupError::InvalidShop);
	S = ValidSetup();
	S.Gold = -5;
	CHECK(StartBattle(S).Error() == BattleSetupError::InvalidShop);
}

int main()
{
	GoldAndShopsAreApplied();
	InvalidShopsAndGoldAreReported();
	ValidSetupStartsOngoingOnTurnOneWithPlayerPhase();
	ConditionsAreApplied();
	InvalidMapIsReported();
	UnplaceableUnitsAreReported();
	BothSidesAreRequired();
	BossIsRequiredWhenTheBossRuleIsOn();
	return TEST_MAIN_RESULT();
}
