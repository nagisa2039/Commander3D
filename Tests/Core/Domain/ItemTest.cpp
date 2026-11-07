#include <vector>

#include "Battle/Battle.h"
#include "TestSupport.h"

using namespace commander::domain;

class AlwaysHit : public IRandomSource
{
public:
	std::int32_t NextPercent() override { return 0; }
};

static constexpr WeaponSpec Sword{3, 100, 1, 1, 0};
static constexpr WeaponSpec Lance{6, 100, 1, 1, 10};
static constexpr WeaponSpec Bow{2, 100, 2, 2, 0};

static ItemSlot WeaponItem(WeaponSpec W, std::int32_t Uses = 0)
{
	ItemSlot S;
	S.Kind = ItemKind::Weapon;
	S.Weapon = W;
	S.Uses = Uses;
	return S;
}

static ItemSlot Potion(std::int32_t Uses, std::int32_t Amount)
{
	ItemSlot S;
	S.Kind = ItemKind::Potion;
	S.Uses = Uses;
	S.Amount = Amount;
	return S;
}

static ItemSlot RepairKit(std::int32_t Uses)
{
	ItemSlot S;
	S.Kind = ItemKind::RepairKit;
	S.Uses = Uses;
	return S;
}

static BattleUnit Unit(std::uint32_t Id, Faction Side, std::int32_t X, std::int32_t Hp, std::vector<ItemSlot> Items = {})
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, 3};
	U.Hp = Hp;
	U.MaxHp = 20;
	U.Strength = 5;
	U.Weapon = Sword;
	U.Items = std::move(Items);
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

static void EquipSwapsTheEquippedWeaponWithTheSlot()
{
	BattleUnit U = Unit(1, Faction::Player, 0, 20, {WeaponItem(Lance, 7)});
	Battle B = MakeBattle({U, Unit(2, Faction::Enemy, 5, 20)});
	const auto E = B.EquipWeapon(UnitId{1}, 0);
	CHECK(E.has_value() && E->Unit == UnitId{1} && E->Slot == 0);
	const BattleUnit* After = B.FindUnit(UnitId{1});
	CHECK(After->Weapon.Might == 6 && After->WeaponUses == 7);
	CHECK(After->Items.size() == 1 && After->Items[0].Kind == ItemKind::Weapon);
	CHECK(After->Items[0].Weapon.Might == 3 && After->Items[0].Uses == 0); // the sword went to the pack
}

static void EquippingDoesNotUseUpTheAction()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {WeaponItem(Bow)}), Unit(2, Faction::Enemy, 2, 20)});
	AlwaysHit Rng;
	CHECK(B.MoveUnit(UnitId{1}, {0, 0}).has_value());
	CHECK(B.EquipWeapon(UnitId{1}, 0).has_value());
	CHECK(!B.FindUnit(UnitId{1})->HasActed);
	CHECK(B.AttackUnit(UnitId{1}, UnitId{2}, Rng).has_value()); // the bow reaches distance 2
}

static void EquipFillsAnEmptyHandAndFreesTheSlot()
{
	BattleUnit U = Unit(1, Faction::Player, 0, 20, {WeaponItem(Lance, 4)});
	U.Weapon = WeaponSpec{};
	Battle B = MakeBattle({U, Unit(2, Faction::Enemy, 5, 20)});
	CHECK(B.EquipWeapon(UnitId{1}, 0).has_value());
	CHECK(B.FindUnit(UnitId{1})->Weapon.Might == 6);
	CHECK(B.FindUnit(UnitId{1})->Items.empty());
}

static void EquipRejections()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 10, {WeaponItem(Lance, 3), Potion(1, 5)}), Unit(2, Faction::Enemy, 3, 20, {WeaponItem(Bow)})});
	CHECK(!B.EquipWeapon(UnitId{1}, 2).has_value());  // no such slot
	CHECK(!B.EquipWeapon(UnitId{1}, 1).has_value());  // a potion, not a weapon
	CHECK(!B.EquipWeapon(UnitId{9}, 0).has_value());  // unknown unit
	CHECK(!B.EquipWeapon(UnitId{2}, 0).has_value());  // not the enemy's phase
	B.WaitUnit(UnitId{1});
	CHECK(!B.EquipWeapon(UnitId{1}, 0).has_value());  // already acted
}

static void AUsedUpWeaponCanBeSwappedOut()
{
	BattleUnit U = Unit(1, Faction::Player, 0, 20, {WeaponItem(Sword)});
	U.Weapon = Lance;
	U.WeaponUses = 0; // broken
	Battle B = MakeBattle({U, Unit(2, Faction::Enemy, 5, 20)});
	CHECK(B.EquipWeapon(UnitId{1}, 0).has_value());
	CHECK(B.FindUnit(UnitId{1})->Weapon.Might == 3);
	CHECK(B.FindUnit(UnitId{1})->Items[0].Weapon.Might == 6 && B.FindUnit(UnitId{1})->Items[0].Uses == 0);
}

static void PotionHealsUpToMaxHpAndEndsTheAction()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 15, {Potion(2, 10)}), Unit(2, Faction::Enemy, 5, 20)});
	const auto E = B.UseItem(UnitId{1}, 0);
	CHECK(E.has_value() && E->Kind == ItemKind::Potion && E->HpHealed == 5); // 15 -> 20, capped by MaxHp
	const BattleUnit* After = B.FindUnit(UnitId{1});
	CHECK(After->Hp == 20 && After->Items.size() == 1 && After->Items[0].Uses == 1);
	CHECK(After->HasActed && After->HasMoved);
	CHECK(!B.WaitUnit(UnitId{1}).has_value());
}

static void AnEmptiedItemIsRemoved()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 5, {Potion(1, 4), WeaponItem(Lance, 2)}), Unit(2, Faction::Enemy, 5, 20)});
	CHECK(B.UseItem(UnitId{1}, 0).has_value());
	const BattleUnit* After = B.FindUnit(UnitId{1});
	CHECK(After->Hp == 9);
	CHECK(After->Items.size() == 1 && After->Items[0].Kind == ItemKind::Weapon);
}

static void PotionIsRefusedAtFullHp()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 20, {Potion(1, 4)}), Unit(2, Faction::Enemy, 5, 20)});
	CHECK(!B.UseItem(UnitId{1}, 0).has_value());
	CHECK(B.FindUnit(UnitId{1})->Items.size() == 1 && !B.FindUnit(UnitId{1})->HasActed);
}

static void RepairKitRestoresTheEquippedWeapon()
{
	BattleUnit U = Unit(1, Faction::Player, 0, 20, {RepairKit(1)});
	U.Weapon = Lance;
	U.WeaponUses = 2;
	Battle B = MakeBattle({U, Unit(2, Faction::Enemy, 5, 20)});
	const auto E = B.UseItem(UnitId{1}, 0);
	CHECK(E.has_value() && E->Kind == ItemKind::RepairKit && E->WeaponRepaired);
	CHECK(B.FindUnit(UnitId{1})->WeaponUses == 10);
	CHECK(B.FindUnit(UnitId{1})->Items.empty());
}

static void RepairKitRejections()
{
	Battle Unbreakable = MakeBattle({Unit(1, Faction::Player, 0, 20, {RepairKit(1)}), Unit(2, Faction::Enemy, 5, 20)});
	CHECK(!Unbreakable.UseItem(UnitId{1}, 0).has_value()); // a sword that never breaks

	BattleUnit Fresh = Unit(1, Faction::Player, 0, 20, {RepairKit(1)});
	Fresh.Weapon = Lance;
	Fresh.WeaponUses = 10;
	Battle Full = MakeBattle({Fresh, Unit(2, Faction::Enemy, 5, 20)});
	CHECK(!Full.UseItem(UnitId{1}, 0).has_value()); // already at full durability
}

static void UseItemRejections()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 10, {WeaponItem(Lance, 3), Potion(1, 5)}), Unit(2, Faction::Enemy, 3, 10, {Potion(1, 5)})});
	CHECK(!B.UseItem(UnitId{1}, 0).has_value()); // a weapon slot cannot be used
	CHECK(!B.UseItem(UnitId{1}, 5).has_value()); // no such slot
	CHECK(!B.UseItem(UnitId{9}, 0).has_value()); // unknown unit
	CHECK(!B.UseItem(UnitId{2}, 0).has_value()); // not the enemy's phase
}

static void NothingWorksAfterTheBattleEnds()
{
	Battle B = MakeBattle({Unit(1, Faction::Player, 0, 10, {Potion(1, 5), WeaponItem(Lance, 3)}), Unit(2, Faction::Enemy, 1, 3)});
	AlwaysHit Rng;
	B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(B.Outcome() == BattleOutcome::PlayerWon);
	CHECK(!B.UseItem(UnitId{1}, 0).has_value());
	CHECK(!B.EquipWeapon(UnitId{1}, 1).has_value());
}

static void AddUnitRejectsInvalidInventories()
{
	Battle B = MakeBattle({});
	CHECK(!B.AddUnit(Unit(1, Faction::Player, 0, 10, std::vector<ItemSlot>(6, Potion(1, 5)))));   // 6 slots
	CHECK(B.AddUnit(Unit(2, Faction::Player, 1, 10, std::vector<ItemSlot>(5, Potion(1, 5)))));    // 5 is fine
	CHECK(!B.AddUnit(Unit(3, Faction::Player, 2, 10, {Potion(0, 5)})));                           // no charges
	CHECK(!B.AddUnit(Unit(4, Faction::Player, 3, 10, {Potion(1, 0)})));                           // heals nothing
	CHECK(!B.AddUnit(Unit(5, Faction::Player, 4, 10, {RepairKit(0)})));
	CHECK(!B.AddUnit(Unit(6, Faction::Player, 5, 10, {WeaponItem(WeaponSpec{})})));               // a weapon with no range
	CHECK(!B.AddUnit(Unit(7, Faction::Player, 0, 10, {WeaponItem(Lance, 11)})));                  // more uses than durability
}

int main()
{
	EquipSwapsTheEquippedWeaponWithTheSlot();
	EquippingDoesNotUseUpTheAction();
	EquipFillsAnEmptyHandAndFreesTheSlot();
	EquipRejections();
	AUsedUpWeaponCanBeSwappedOut();
	PotionHealsUpToMaxHpAndEndsTheAction();
	AnEmptiedItemIsRemoved();
	PotionIsRefusedAtFullHp();
	RepairKitRestoresTheEquippedWeapon();
	RepairKitRejections();
	UseItemRejections();
	NothingWorksAfterTheBattleEnds();
	AddUnitRejectsInvalidInventories();
	return TEST_MAIN_RESULT();
}
