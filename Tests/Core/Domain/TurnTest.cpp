#include "Battle/Battle.h"
#include "TestSupport.h"

using namespace commander::domain;

static Battle MakeBattle()
{
	const auto Map = BattleMap::Create(*Grid::Create(5, 1), {{1, true}}, {0, 0, 0, 0, 0});
	Battle B(*Map);
	B.AddUnit(BattleUnit{UnitId{1}, Faction::Player, {0, 0}, 1});
	B.AddUnit(BattleUnit{UnitId{2}, Faction::Enemy, {4, 0}, 1});
	return B;
}

static void StartsWithPlayerPhaseOnTurnOne()
{
	const Battle B = MakeBattle();
	CHECK(B.CurrentFaction() == Faction::Player);
	CHECK(B.TurnNumber() == 1);
}

static void OnlyCurrentFactionMayMove()
{
	Battle B = MakeBattle();
	CHECK(!B.MoveUnit(UnitId{2}, {3, 0}).has_value());
	CHECK(B.MoveUnit(UnitId{1}, {1, 0}).has_value());
}

static void UnitMovesOnlyOncePerPhase()
{
	Battle B = MakeBattle();
	CHECK(B.MoveUnit(UnitId{1}, {1, 0}).has_value());
	CHECK(B.FindUnit(UnitId{1})->HasMoved);
	CHECK(!B.MoveUnit(UnitId{1}, {0, 0}).has_value());
	CHECK(B.FindUnit(UnitId{1})->Position == (GridCoord{1, 0}));
}

static void WaitEndsTheUnitsAction()
{
	Battle B = MakeBattle();
	const auto Event = B.WaitUnit(UnitId{1});
	CHECK(Event.has_value());
	CHECK(Event->Unit == UnitId{1});
	CHECK(B.FindUnit(UnitId{1})->HasActed);
	CHECK(!B.WaitUnit(UnitId{1}).has_value());
	CHECK(!B.MoveUnit(UnitId{1}, {1, 0}).has_value());
}

static void WaitRejectsUnknownUnitAndOtherFaction()
{
	Battle B = MakeBattle();
	CHECK(!B.WaitUnit(UnitId{9}).has_value());
	CHECK(!B.WaitUnit(UnitId{2}).has_value());
}

static void EndPhaseSwitchesFactionAndResetsFlags()
{
	Battle B = MakeBattle();
	B.MoveUnit(UnitId{1}, {1, 0});
	B.WaitUnit(UnitId{1});
	const PhaseChanged E1 = B.EndPhase();
	CHECK(E1.Side == Faction::Enemy);
	CHECK(E1.Turn == 1);
	CHECK(B.CurrentFaction() == Faction::Enemy);
	CHECK(!B.FindUnit(UnitId{1})->HasMoved);
	CHECK(!B.FindUnit(UnitId{1})->HasActed);
	CHECK(B.MoveUnit(UnitId{2}, {3, 0}).has_value());
	CHECK(!B.MoveUnit(UnitId{1}, {2, 0}).has_value());
}

static void TurnAdvancesWhenPlayerPhaseReturns()
{
	Battle B = MakeBattle();
	B.EndPhase();
	const PhaseChanged E = B.EndPhase();
	CHECK(E.Side == Faction::Player);
	CHECK(E.Turn == 2);
	CHECK(B.TurnNumber() == 2);
	CHECK(B.MoveUnit(UnitId{1}, {1, 0}).has_value());
}

int main()
{
	StartsWithPlayerPhaseOnTurnOne();
	OnlyCurrentFactionMayMove();
	UnitMovesOnlyOncePerPhase();
	WaitEndsTheUnitsAction();
	WaitRejectsUnknownUnitAndOtherFaction();
	EndPhaseSwitchesFactionAndResetsFlags();
	TurnAdvancesWhenPlayerPhaseReturns();
	return TEST_MAIN_RESULT();
}
