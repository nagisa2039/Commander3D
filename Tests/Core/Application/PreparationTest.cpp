#include <vector>

#include "Battle/Preparation.h"
#include "TestSupport.h"

using namespace commander::application;
using namespace commander::domain;

static ItemSlot Potion()
{
	ItemSlot S;
	S.Kind = ItemKind::Potion;
	S.Uses = 1;
	S.Amount = 5;
	return S;
}

static BattleUnit Member(std::uint32_t Id)
{
	BattleUnit U{UnitId{Id}, Faction::Player, {0, 0}, 3};
	U.Hp = 20;
	U.MaxHp = 20;
	return U;
}

static BattleUnit Boss()
{
	BattleUnit U{UnitId{100}, Faction::Enemy, {3, 0}, 3};
	U.IsBoss = true;
	return U;
}

// 4x1 map; enemy at x=3; player start cells x=0 and x=1.
static BattleSetup BaseSetup()
{
	BattleSetup S;
	S.Map.Width = 4;
	S.Map.Height = 1;
	S.Map.Terrains = {{1, true}, {0, false}};
	S.Map.Cells = {0, 0, 0, 0};
	S.Units = {Boss()};
	return S;
}

static Roster MakeRoster()
{
	Roster R;
	BattleUnit A = Member(1);
	A.Items = {Potion()};
	R.Add(A);
	R.Add(Member(2));
	R.Add(Member(3));
	R.SetGold(120);
	return R;
}

static Preparation Make()
{
	return Preparation(MakeRoster(), BaseSetup(), {GridCoord{0, 0}, GridCoord{1, 0}});
}

static void StartsWithNoOneDeployed()
{
	Preparation P = Make();
	CHECK(P.Deployed().empty());
	CHECK(P.MaxDeployed() == 2);
	CHECK(P.Start().Error() == PreparationError::NoUnitDeployed);
}

static void DeploymentIsLimitedByStartCells()
{
	Preparation P = Make();
	CHECK(P.Deploy(UnitId{1}));
	CHECK(!P.Deploy(UnitId{1})); // already deployed
	CHECK(P.Deploy(UnitId{2}));
	CHECK(!P.Deploy(UnitId{3})); // full
	CHECK(!P.Deploy(UnitId{9})); // unknown
	CHECK(P.Withdraw(UnitId{2}));
	CHECK(!P.Withdraw(UnitId{2}));
	CHECK(P.Deploy(UnitId{3}));
	CHECK(P.Deployed().size() == 2);
}

static void StartPlacesDeployedUnitsOnStartCellsInOrder()
{
	Preparation P = Make();
	P.Deploy(UnitId{3});
	P.Deploy(UnitId{1});
	const auto R = P.Start();
	CHECK(R.IsOk());
	const Battle& B = R.Value().Battle;
	CHECK(B.Units().size() == 3); // boss + 2
	CHECK(B.Gold() == 120);
	CHECK(R.Value().Deployed.size() == 2);
	for (const BattleUnit& U : B.Units())
	{
		if (U.Id == UnitId{3})
		{
			CHECK(U.Position == (GridCoord{0, 0}));
		}
		if (U.Id == UnitId{1})
		{
			CHECK(U.Position == (GridCoord{1, 0}));
		}
	}
}

static void ItemChangesCarryIntoTheBattle()
{
	Preparation P = Make();
	CHECK(P.TransferItem(UnitId{1}, UnitId{2}, 0));
	P.Deploy(UnitId{2});
	const auto R = P.Start();
	CHECK(R.IsOk());
	for (const BattleUnit& U : R.Value().Battle.Units())
	{
		if (U.Id == UnitId{2})
		{
			CHECK(U.Items.size() == 1);
		}
	}
	// the preparation's own roster is not consumed by starting
	CHECK(P.Roster().Find(UnitId{2})->Items.size() == 1);
}

static void DiscardIsAvailable()
{
	Preparation P = Make();
	CHECK(P.DiscardItem(UnitId{1}, 0));
	CHECK(!P.DiscardItem(UnitId{1}, 0));
}

static Preparation MakeWithStock()
{
	ItemSlot Priced = Potion();
	Priced.Price = 100;
	BattleSetup S = BaseSetup();
	S.PreparationStock = {Priced};
	return Preparation(MakeRoster(), S, {GridCoord{0, 0}, GridCoord{1, 0}});
}

static void ShoppingUsesTheRosterGoldAndStock()
{
	Preparation P = MakeWithStock(); // roster gold 120
	CHECK(P.Stock().size() == 1);
	CHECK(P.BuyItem(UnitId{2}, 0));
	CHECK(P.Roster().Gold() == 20);
	CHECK(P.Roster().Find(UnitId{2})->Items.size() == 1);
	CHECK(!P.BuyItem(UnitId{2}, 0)); // short of gold
	CHECK(!P.BuyItem(UnitId{2}, 4)); // no such entry
	CHECK(P.SellItem(UnitId{2}, 0)); // half of 100
	CHECK(P.Roster().Gold() == 70);

	// what was bought is taken into the battle, along with the remaining gold
	CHECK(!P.BuyItem(UnitId{1}, 0)); // 70 gold is short of 100
	P.Deploy(UnitId{2});
	const auto R = P.Start();
	CHECK(R.IsOk());
	CHECK(R.Value().Battle.Gold() == 70);
}

static void NoStockMeansNoShopping()
{
	Preparation P = Make();
	CHECK(P.Stock().empty());
	CHECK(!P.BuyItem(UnitId{1}, 0));
}

static void BadSetupIsReported()
{
	BattleSetup S = BaseSetup();
	S.Units.clear(); // no enemy
	Preparation P(MakeRoster(), S, {GridCoord{0, 0}});
	P.Deploy(UnitId{1});
	CHECK(P.Start().Error() == PreparationError::InvalidSetup);
}

static void FinishCarriesResultBackToTheRoster()
{
	Preparation P = Make();
	P.Deploy(UnitId{1});
	P.Deploy(UnitId{2});
	auto Started = P.Start();
	CHECK(Started.IsOk());
	Battle B = Started.Value().Battle;
	B.SetGold(500);

	P.Finish(B, Started.Value().Deployed, Difficulty::Classic);
	CHECK(P.Roster().Gold() == 500);
	CHECK(P.Roster().Members().size() == 3);
}

int main()
{
	StartsWithNoOneDeployed();
	DeploymentIsLimitedByStartCells();
	StartPlacesDeployedUnitsOnStartCellsInOrder();
	ItemChangesCarryIntoTheBattle();
	DiscardIsAvailable();
	ShoppingUsesTheRosterGoldAndStock();
	NoStockMeansNoShopping();
	BadSetupIsReported();
	FinishCarriesResultBackToTheRoster();
	return TEST_MAIN_RESULT();
}
