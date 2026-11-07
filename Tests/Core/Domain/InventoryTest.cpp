#include <vector>

#include "Battle/Battle.h"
#include "TestSupport.h"

using namespace commander::domain;

static ItemSlot Potion(std::int32_t Uses = 1, std::int32_t Amount = 5, std::int32_t Price = 0)
{
	ItemSlot S;
	S.Kind = ItemKind::Potion;
	S.Uses = Uses;
	S.Amount = Amount;
	S.Price = Price;
	return S;
}

static ItemSlot Staff(std::int32_t Uses, std::int32_t Amount, std::int32_t Range)
{
	ItemSlot S;
	S.Kind = ItemKind::Staff;
	S.Uses = Uses;
	S.Amount = Amount;
	S.Range = Range;
	return S;
}

static BattleUnit Unit(std::uint32_t Id, Faction Side, std::int32_t X, std::int32_t Hp = 20, std::vector<ItemSlot> Items = {})
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, 3};
	U.Hp = Hp;
	U.MaxHp = 20;
	U.Items = std::move(Items);
	return U;
}

static Battle MakeBattle(const std::vector<BattleUnit>& Units)
{
	// 6x1, cell 5 is a wall.
	const auto Map = BattleMap::Create(*Grid::Create(6, 1), {{1, true}, {0, false}}, {0, 0, 0, 0, 0, 1});
	Battle B(*Map);
	for (const BattleUnit& U : Units)
	{
		B.AddUnit(U);
	}
	return B;
}

// Returns the given rolls in order, repeating the last one.
class FixedRandom : public IRandomSource
{
public:
	explicit FixedRandom(std::vector<std::int32_t> InRolls) : Rolls(std::move(InRolls)) {}
	std::int32_t NextPercent() override
	{
		const std::int32_t V = Rolls[Next < Rolls.size() ? Next : Rolls.size() - 1];
		++Next;
		return V;
	}

private:
	std::vector<std::int32_t> Rolls;
	std::size_t Next = 0;
};

// Always rolls 0 (used by tests that do not care about randomness).
static FixedRandom Rng({0});

// ---- give / drop ----

static void GiveMovesTheItemToAnAdjacentAlly()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Potion(2), Potion(1, 9)}), Unit(2, Faction::Player, 1)});
	const auto E = B.GiveItem(UnitId{1}, UnitId{2}, 1);
	CHECK(E.has_value() && E->Giver == UnitId{1} && E->Receiver == UnitId{2} && E->Slot == 1);
	CHECK(B.FindUnit(UnitId{1})->Items.size() == 1);
	CHECK(B.FindUnit(UnitId{2})->Items.size() == 1 && B.FindUnit(UnitId{2})->Items[0].Amount == 9);
	CHECK(B.FindUnit(UnitId{1})->HasActed && !B.FindUnit(UnitId{2})->HasActed);
}

static void GiveRejections()
{
	std::vector<ItemSlot> Full(5, Potion());
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Potion()}), Unit(2, Faction::Player, 2), Unit(3, Faction::Enemy, 1),
		Unit(4, Faction::Player, 3, 20, Full)});
	CHECK(!B.GiveItem(UnitId{1}, UnitId{2}, 0).has_value()); // distance 2
	CHECK(!B.GiveItem(UnitId{1}, UnitId{3}, 0).has_value()); // enemy
	CHECK(!B.GiveItem(UnitId{1}, UnitId{1}, 0).has_value()); // self
	CHECK(!B.GiveItem(UnitId{1}, UnitId{9}, 0).has_value()); // unknown receiver
	CHECK(!B.GiveItem(UnitId{9}, UnitId{2}, 0).has_value()); // unknown giver
	CHECK(!B.GiveItem(UnitId{1}, UnitId{2}, 3).has_value()); // no such slot
	CHECK(B.FindUnit(UnitId{1})->Items.size() == 1 && !B.FindUnit(UnitId{1})->HasActed);

	Battle C = MakeBattle({Unit(1, Faction::Player, 0, 20, {Potion()}), Unit(4, Faction::Player, 1, 20, Full)});
	CHECK(!C.GiveItem(UnitId{1}, UnitId{4}, 0).has_value()); // receiver's pack is full
	C.WaitUnit(UnitId{1});
	CHECK(!C.GiveItem(UnitId{1}, UnitId{4}, 0).has_value()); // already acted
}

static void DropRemovesTheItemWithoutUsingTheAction()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Potion(), Potion(1, 8)}), Unit(2, Faction::Enemy, 4)});
	const auto E = B.DropItem(UnitId{1}, 0);
	CHECK(E.has_value() && E->Unit == UnitId{1} && E->Slot == 0);
	CHECK(B.FindUnit(UnitId{1})->Items.size() == 1 && B.FindUnit(UnitId{1})->Items[0].Amount == 8);
	CHECK(!B.FindUnit(UnitId{1})->HasActed);
	CHECK(!B.DropItem(UnitId{1}, 5).has_value());
	CHECK(!B.DropItem(UnitId{9}, 0).has_value());
	CHECK(!B.DropItem(UnitId{2}, 0).has_value()); // not the enemy's phase
}

// ---- staffs ----

static void AStaffHealsAnAllyInRange()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Staff(2, 8, 2)}), Unit(2, Faction::Player, 2, 5)});
	const auto E = B.UseStaff(UnitId{1}, UnitId{2}, 0, Rng);
	CHECK(E.has_value() && E->Healer == UnitId{1} && E->Target == UnitId{2} && E->HpHealed == 8);
	CHECK(B.FindUnit(UnitId{2})->Hp == 13);
	CHECK(B.FindUnit(UnitId{1})->Items[0].Uses == 1);
	CHECK(B.FindUnit(UnitId{1})->HasActed && B.FindUnit(UnitId{1})->HasMoved);
}

static void AStaffHealsOnlyWhatIsMissingAndBreaksAtZeroUses()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Staff(1, 30, 1)}), Unit(2, Faction::Player, 1, 17)});
	const auto E = B.UseStaff(UnitId{1}, UnitId{2}, 0, Rng);
	CHECK(E.has_value() && E->HpHealed == 3);
	CHECK(B.FindUnit(UnitId{2})->Hp == 20);
	CHECK(B.FindUnit(UnitId{1})->Items.empty());
}

static void StaffRejections()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Staff(3, 8, 2), Potion()}), Unit(2, Faction::Player, 3, 5),
		Unit(3, Faction::Enemy, 1, 5), Unit(4, Faction::Player, 2, 20)});
	CHECK(!B.UseStaff(UnitId{1}, UnitId{2}, 0, Rng).has_value()); // distance 3, range 2
	CHECK(!B.UseStaff(UnitId{1}, UnitId{3}, 0, Rng).has_value()); // enemy
	CHECK(!B.UseStaff(UnitId{1}, UnitId{4}, 0, Rng).has_value()); // already at full Hp
	CHECK(!B.UseStaff(UnitId{1}, UnitId{1}, 0, Rng).has_value()); // not itself
	CHECK(!B.UseStaff(UnitId{1}, UnitId{9}, 0, Rng).has_value()); // unknown target
	CHECK(!B.UseStaff(UnitId{1}, UnitId{4}, 1, Rng).has_value()); // slot 1 is a potion
	CHECK(!B.UseStaff(UnitId{1}, UnitId{4}, 7, Rng).has_value()); // no such slot
	CHECK(B.FindUnit(UnitId{1})->Items[0].Uses == 3 && !B.FindUnit(UnitId{1})->HasActed);
}

static const ExperienceGained* StaffGain(const std::optional<StaffUsed>& E)
{
	return (E.has_value() && E->Experience.has_value()) ? &*E->Experience : nullptr;
}

static void UsingAStaffEarnsExperience()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Staff(2, 8, 2)}), Unit(2, Faction::Player, 2, 5)});
	const auto E = B.UseStaff(UnitId{1}, UnitId{2}, 0, Rng);
	CHECK(StaffGain(E) != nullptr && StaffGain(E)->Unit == UnitId{1} && StaffGain(E)->Amount == 12); // 8 + 8 / 2
	CHECK(B.FindUnit(UnitId{1})->Experience == 12);
	CHECK(!E->LevelUp.has_value());
}

static void StaffExperienceDependsOnTheHpHealed()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Staff(2, 30, 2)}), Unit(2, Faction::Player, 2, 17)});
	const auto E = B.UseStaff(UnitId{1}, UnitId{2}, 0, Rng); // only 3 Hp were missing
	CHECK(StaffGain(E) != nullptr && StaffGain(E)->Amount == 9); // 8 + 3 / 2
}

static void StaffExperienceCanLevelTheHealerUp()
{
	BattleUnit Healer = Unit(1, Faction::Player, 0, 20, {Staff(2, 8, 2)});
	Healer.Experience = 95;
	Healer.Growth = GrowthRates{100, 0, 100, 0, 100};
	Battle B = MakeBattle({Healer, Unit(2, Faction::Player, 2, 5)});
	FixedRandom Rolls({0});
	const auto E = B.UseStaff(UnitId{1}, UnitId{2}, 0, Rolls);
	CHECK(E.has_value() && E->LevelUp.has_value());
	CHECK(E->LevelUp->NewLevel == 2 && E->LevelUp->HpGain == 1 && E->LevelUp->StrengthGain == 0 && E->LevelUp->SkillGain == 1);
	CHECK(B.FindUnit(UnitId{1})->Level == 2 && B.FindUnit(UnitId{1})->Experience == 7); // 95 + 12 - 100
}

static void EnemyHealersAndMaxLevelHealersEarnNothing()
{
	BattleUnit EnemyHealer = Unit(5, Faction::Enemy, 3, 20, {Staff(2, 8, 2)});
	Battle B = MakeBattle({Unit(1, Faction::Player, 0), EnemyHealer, Unit(6, Faction::Enemy, 4, 5)});
	B.EndPhase();
	const auto E = B.UseStaff(UnitId{5}, UnitId{6}, 0, Rng);
	CHECK(E.has_value() && !E->Experience.has_value() && !E->LevelUp.has_value());
	CHECK(B.FindUnit(UnitId{5})->Experience == 0);

	BattleUnit Veteran = Unit(1, Faction::Player, 0, 20, {Staff(2, 8, 2)});
	Veteran.Level = MaxLevel;
	Battle C = MakeBattle({Veteran, Unit(2, Faction::Player, 2, 5)});
	const auto F = C.UseStaff(UnitId{1}, UnitId{2}, 0, Rng);
	CHECK(F.has_value() && !F->Experience.has_value());
}

// ---- shops ----

static Shop MakeShop(std::int32_t X, std::vector<ItemSlot> Stock)
{
	Shop S;
	S.Position = {X, 0};
	S.Stock = std::move(Stock);
	return S;
}

static void AddShopValidation()
{
	Battle B = MakeBattle({});
	CHECK(B.AddShop(MakeShop(1, {Potion(1, 5, 30)})));
	CHECK(!B.AddShop(MakeShop(1, {Potion(1, 5, 30)})));  // already a shop there
	CHECK(!B.AddShop(MakeShop(9, {Potion(1, 5, 30)})));  // outside the grid
	CHECK(!B.AddShop(MakeShop(5, {Potion(1, 5, 30)})));  // wall
	CHECK(!B.AddShop(MakeShop(2, {Potion(1, 5, 0)})));   // free items are not for sale
	CHECK(!B.AddShop(MakeShop(2, {Potion(0, 5, 30)})));  // invalid item
	CHECK(!B.AddShop(MakeShop(2, {})));                  // empty stock
	CHECK(B.Shops().size() == 1);
	CHECK(B.SetGold(100) && B.Gold() == 100);
	CHECK(!B.SetGold(-1) && B.Gold() == 100);
}

static Battle ShopBattle(std::int32_t Gold)
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 1), Unit(2, Faction::Player, 3), Unit(3, Faction::Enemy, 4)});
	B.AddShop(MakeShop(1, {Potion(3, 10, 40), Potion(1, 5, 25)}));
	B.SetGold(Gold);
	return B;
}

static void BuyingAtTheShopSpendsGoldAndFillsThePack()
{
	Battle B = ShopBattle(100);
	const auto E = B.BuyItem(UnitId{1}, 0);
	CHECK(E.has_value() && E->Unit == UnitId{1} && E->Entry == 0 && E->Price == 40);
	CHECK(B.Gold() == 60);
	CHECK(B.FindUnit(UnitId{1})->Items.size() == 1 && B.FindUnit(UnitId{1})->Items[0].Uses == 3);
	CHECK(!B.FindUnit(UnitId{1})->HasActed); // shopping is free
	CHECK(B.Shops()[0].Stock.size() == 2);   // the stock never runs out
}

static void BuyRejections()
{
	Battle Poor = ShopBattle(39);
	CHECK(!Poor.BuyItem(UnitId{1}, 0).has_value());                  // not enough gold
	CHECK(Poor.BuyItem(UnitId{1}, 1).has_value());                   // the 25-gold item is affordable
	Battle B = ShopBattle(1000);
	CHECK(!B.BuyItem(UnitId{2}, 0).has_value());                     // unit 2 is not on the shop cell
	CHECK(!B.BuyItem(UnitId{1}, 2).has_value());                     // no such entry
	CHECK(!B.BuyItem(UnitId{9}, 0).has_value());                     // unknown unit
	B.EndPhase();
	CHECK(!B.BuyItem(UnitId{3}, 0).has_value());                     // the enemy is not on a shop and not the player's side
	Battle Full = MakeBattle({Unit(1, Faction::Player, 1, 20, std::vector<ItemSlot>(5, Potion()))});
	Full.AddShop(MakeShop(1, {Potion(1, 5, 10)}));
	Full.SetGold(100);
	CHECK(!Full.BuyItem(UnitId{1}, 0).has_value());                  // pack full
	CHECK(Full.Gold() == 100);
}

static void SellingPaysHalfThePrice()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 1, 20, {Potion(1, 5, 41), Potion(1, 5, 0)}), Unit(2, Faction::Player, 3, 20, {Potion(1, 5, 40)})});
	B.AddShop(MakeShop(1, {Potion(1, 5, 10)}));
	const auto E = B.SellItem(UnitId{1}, 0);
	CHECK(E.has_value() && E->Slot == 0 && E->Gold == 20); // 41 / 2, rounded down
	CHECK(B.Gold() == 20 && B.FindUnit(UnitId{1})->Items.size() == 1);
	CHECK(!B.SellItem(UnitId{1}, 0).has_value());          // the remaining item has no price: cannot be sold
	CHECK(!B.SellItem(UnitId{1}, 4).has_value());          // no such slot
	CHECK(!B.SellItem(UnitId{2}, 0).has_value());          // not on a shop cell
}

static void ShopsAndGoldStopWorkingAfterTheBattleEnds()
{
	// A player unit defeats the only enemy, which ends the battle.
	BattleUnit Striker = Unit(5, Faction::Player, 2);
	Striker.Strength = 20;
	Striker.Weapon = WeaponSpec{5, 100, 1, 1};
	Battle C = MakeBattle({Unit(1, Faction::Player, 1), Striker, Unit(3, Faction::Enemy, 3, 5)});
	C.AddShop(MakeShop(1, {Potion(1, 5, 10)}));
	C.SetGold(100);
	C.AttackUnit(UnitId{5}, UnitId{3}, Rng);
	CHECK(C.Outcome() == BattleOutcome::PlayerWon);
	CHECK(!C.BuyItem(UnitId{1}, 0).has_value());
}

int main()
{
	GiveMovesTheItemToAnAdjacentAlly();
	GiveRejections();
	DropRemovesTheItemWithoutUsingTheAction();
	AStaffHealsAnAllyInRange();
	AStaffHealsOnlyWhatIsMissingAndBreaksAtZeroUses();
	StaffRejections();
	UsingAStaffEarnsExperience();
	StaffExperienceDependsOnTheHpHealed();
	StaffExperienceCanLevelTheHealerUp();
	EnemyHealersAndMaxLevelHealersEarnNothing();
	AddShopValidation();
	BuyingAtTheShopSpendsGoldAndFillsThePack();
	BuyRejections();
	SellingPaysHalfThePrice();
	ShopsAndGoldStopWorkingAfterTheBattleEnds();
	return TEST_MAIN_RESULT();
}
