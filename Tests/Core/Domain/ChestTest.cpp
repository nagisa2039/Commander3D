#include <vector>

#include "Battle/Battle.h"
#include "TestSupport.h"

using namespace commander::domain;

static ItemSlot Potion()
{
	ItemSlot S;
	S.Kind = ItemKind::Potion;
	S.Uses = 2;
	S.Amount = 5;
	return S;
}

static Chest ItemChest(std::int32_t X, ItemSlot Item = Potion())
{
	Chest C;
	C.Position = {X, 0};
	C.Item = Item;
	return C;
}

static Chest GoldChest(std::int32_t X, std::int32_t Gold)
{
	Chest C;
	C.Position = {X, 0};
	C.Gold = Gold;
	return C;
}

static BattleUnit Unit(std::uint32_t Id, Faction Side, std::int32_t X, std::vector<ItemSlot> Items = {})
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, 3};
	U.Hp = 10;
	U.MaxHp = 10;
	U.Items = std::move(Items);
	return U;
}

// 6x1, cell 5 is a wall. Player unit 1 at x=1, enemy unit 9 at x=4.
static Battle MakeBattle(std::vector<ItemSlot> Pack = {})
{
	const auto Map = BattleMap::Create(*Grid::Create(6, 1), {{1, true}, {0, false}}, {0, 0, 0, 0, 0, 1});
	Battle B(*Map);
	B.AddUnit(Unit(1, Faction::Player, 1, std::move(Pack)));
	B.AddUnit(Unit(9, Faction::Enemy, 4));
	return B;
}

static void AddChestValidation()
{
	Battle B = MakeBattle();
	CHECK(B.AddChest(ItemChest(2)));
	CHECK(B.AddChest(GoldChest(3, 200)));
	CHECK(B.Chests().size() == 2);

	CHECK(!B.AddChest(ItemChest(2)));                      // cell already has a chest
	CHECK(!B.AddChest(ItemChest(5)));                      // wall
	CHECK(!B.AddChest(ItemChest(6)));                      // outside the grid
	CHECK(!B.AddChest(Chest{GridCoord{0, 0}, std::nullopt, 0})); // empty
	Chest Both = ItemChest(0);
	Both.Gold = 10;
	CHECK(!B.AddChest(Both));                              // item and gold
	CHECK(!B.AddChest(GoldChest(0, -5)));                  // negative gold
	ItemSlot Bad = Potion();
	Bad.Uses = 0;
	CHECK(!B.AddChest(ItemChest(0, Bad)));                 // invalid item
	CHECK(B.Chests().size() == 2);
}

static void OpeningAnItemChestGivesTheItem()
{
	Battle B = MakeBattle();
	B.AddChest(ItemChest(2));
	CHECK(B.MoveUnit(UnitId{1}, {2, 0}).has_value());

	const auto Event = B.OpenChest(UnitId{1});
	CHECK(Event.has_value());
	CHECK(Event->Unit == UnitId{1});
	CHECK(Event->Position == (GridCoord{2, 0}));
	CHECK(Event->Item.has_value() && Event->Item->Kind == ItemKind::Potion);
	CHECK(Event->Gold == 0);

	CHECK(B.FindUnit(UnitId{1})->Items.size() == 1);
	CHECK(B.Chests().empty());
	CHECK(B.FindUnit(UnitId{1})->HasActed); // opening ends the action
	CHECK(!B.OpenChest(UnitId{1}).has_value());
}

static void OpeningAGoldChestAddsToTheArmyGold()
{
	Battle B = MakeBattle();
	B.SetGold(100);
	B.AddChest(GoldChest(1, 250)); // right under the unit
	const auto Event = B.OpenChest(UnitId{1});
	CHECK(Event.has_value() && !Event->Item.has_value() && Event->Gold == 250);
	CHECK(B.Gold() == 350);
	CHECK(B.FindUnit(UnitId{1})->Items.empty());
}

static void OpeningIsRefusedWhenNotPossible()
{
	// nothing under the unit
	Battle B = MakeBattle();
	B.AddChest(ItemChest(2));
	CHECK(!B.OpenChest(UnitId{1}).has_value());
	CHECK(!B.OpenChest(UnitId{77}).has_value()); // unknown unit
	CHECK(B.Chests().size() == 1);

	// a full pack cannot take an item, and the chest stays
	Battle Full = MakeBattle({Potion(), Potion(), Potion(), Potion(), Potion()});
	Full.AddChest(ItemChest(1));
	CHECK(!Full.OpenChest(UnitId{1}).has_value());
	CHECK(Full.Chests().size() == 1);
	CHECK(!Full.FindUnit(UnitId{1})->HasActed);

	// a full pack does not matter for gold
	Battle FullGold = MakeBattle({Potion(), Potion(), Potion(), Potion(), Potion()});
	FullGold.AddChest(GoldChest(1, 50));
	CHECK(FullGold.OpenChest(UnitId{1}).has_value());

	// a unit that already acted cannot open it
	Battle Acted = MakeBattle();
	Acted.AddChest(ItemChest(1));
	Acted.WaitUnit(UnitId{1});
	CHECK(!Acted.OpenChest(UnitId{1}).has_value());
	CHECK(Acted.Chests().size() == 1);
}

static void EnemiesDoNotOpenChests()
{
	Battle B = MakeBattle();
	B.AddChest(ItemChest(4)); // under the enemy
	B.EndPhase();             // enemy phase
	CHECK(!B.OpenChest(UnitId{9}).has_value());
	CHECK(B.Chests().size() == 1);
}

int main()
{
	AddChestValidation();
	OpeningAnItemChestGivesTheItem();
	OpeningAGoldChestAddsToTheArmyGold();
	OpeningIsRefusedWhenNotPossible();
	EnemiesDoNotOpenChests();
	return TEST_MAIN_RESULT();
}
