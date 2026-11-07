#include <vector>

#include "Battle/Battle.h"
#include "Roster/Roster.h"
#include "TestSupport.h"

using namespace commander::domain;

static ItemSlot Potion()
{
	ItemSlot S;
	S.Kind = ItemKind::Potion;
	S.Uses = 1;
	S.Amount = 5;
	return S;
}

static BattleUnit Member(std::uint32_t Id, std::int32_t Level = 1)
{
	BattleUnit U{UnitId{Id}, Faction::Player, {0, 0}, 3};
	U.Hp = 20;
	U.MaxHp = 20;
	U.Level = Level;
	return U;
}

static BattleUnit Enemy(std::uint32_t Id, std::int32_t X)
{
	BattleUnit U{UnitId{Id}, Faction::Enemy, {X, 0}, 3};
	U.Hp = 10;
	U.MaxHp = 10;
	return U;
}

static Battle MakeBattle(const std::vector<BattleUnit>& Units)
{
	const auto Map = BattleMap::Create(*Grid::Create(6, 1), {{1, true}, {0, false}}, {0, 0, 0, 0, 0, 1});
	Battle B(*Map);
	for (const BattleUnit& U : Units)
	{
		B.AddUnit(U);
	}
	return B;
}

static void TestAdd()
{
	Roster R;
	CHECK(R.Add(Member(1)));
	CHECK(!R.Add(Member(1))); // duplicate id

	BattleUnit EnemyUnit = Member(2);
	EnemyUnit.Side = Faction::Enemy;
	CHECK(!R.Add(EnemyUnit)); // players only

	BattleUnit Bad = Member(3);
	Bad.Level = 0;
	CHECK(!R.Add(Bad)); // invalid data

	CHECK(R.Members().size() == 1);
	CHECK(R.Find(UnitId{1}) != nullptr);
	CHECK(R.Find(UnitId{9}) == nullptr);
}

static void TestGold()
{
	Roster R;
	CHECK(R.Gold() == 0);
	CHECK(R.SetGold(300));
	CHECK(R.Gold() == 300);
	CHECK(!R.SetGold(-1));
	CHECK(R.Gold() == 300);
}

static void TestDeploy()
{
	Roster R;
	BattleUnit M = Member(1);
	M.Hp = 5;
	M.HasActed = true;
	R.Add(M);

	const auto Deployed = R.Deploy(UnitId{1}, GridCoord{2, 0});
	CHECK(Deployed.has_value());
	CHECK(Deployed->Position == (GridCoord{2, 0}));
	CHECK(Deployed->Hp == Deployed->MaxHp); // healed between battles
	CHECK(!Deployed->HasActed && !Deployed->HasMoved);
	CHECK(!R.Deploy(UnitId{9}, GridCoord{0, 0}).has_value());
}

static void TestItemOrganizing()
{
	Roster R;
	BattleUnit A = Member(1);
	A.Items = {Potion(), Potion()};
	R.Add(A);
	R.Add(Member(2));

	CHECK(R.TransferItem(UnitId{1}, UnitId{2}, 0));
	CHECK(R.Find(UnitId{1})->Items.size() == 1);
	CHECK(R.Find(UnitId{2})->Items.size() == 1);

	CHECK(!R.TransferItem(UnitId{1}, UnitId{1}, 0)); // itself
	CHECK(!R.TransferItem(UnitId{1}, UnitId{2}, 5)); // no such slot
	CHECK(!R.TransferItem(UnitId{1}, UnitId{9}, 0)); // unknown receiver

	CHECK(R.DiscardItem(UnitId{1}, 0));
	CHECK(R.Find(UnitId{1})->Items.empty());
	CHECK(!R.DiscardItem(UnitId{1}, 0));

	// a full pack refuses
	BattleUnit Full = Member(3);
	Full.Items = {Potion(), Potion(), Potion(), Potion(), Potion()};
	R.Add(Full);
	CHECK(!R.TransferItem(UnitId{2}, UnitId{3}, 0));
	CHECK(R.Find(UnitId{2})->Items.size() == 1);
}

static ItemSlot Sword(std::int32_t Might)
{
	ItemSlot S;
	S.Kind = ItemKind::Weapon;
	S.Weapon.Might = Might;
	S.Weapon.MinRange = 1;
	S.Weapon.MaxRange = 1;
	return S;
}

static void TestEquipWeapon()
{
	Roster R;
	BattleUnit A = Member(1);
	A.Weapon.Might = 5;
	A.Weapon.MinRange = 1;
	A.Weapon.MaxRange = 1;
	A.Items = {Potion(), Sword(9)};
	R.Add(A);

	CHECK(!R.EquipWeapon(UnitId{1}, 0));  // a potion
	CHECK(!R.EquipWeapon(UnitId{1}, 7));  // no such slot
	CHECK(!R.EquipWeapon(UnitId{9}, 1));  // unknown member
	CHECK(R.EquipWeapon(UnitId{1}, 1));
	CHECK(R.Find(UnitId{1})->Weapon.Might == 9);
	CHECK(R.Find(UnitId{1})->Items[1].Weapon.Might == 5); // swapped into the pack

	// an empty hand takes the weapon and frees the slot
	BattleUnit Bare = Member(2);
	Bare.Items = {Sword(7)};
	R.Add(Bare);
	CHECK(R.EquipWeapon(UnitId{2}, 0));
	CHECK(R.Find(UnitId{2})->Weapon.Might == 7);
	CHECK(R.Find(UnitId{2})->Items.empty());
}

static void TestBuyAndSell()
{
	ItemSlot Stocked = Potion();
	Stocked.Price = 300;
	const std::vector<ItemSlot> Stock = {Stocked};

	Roster R;
	R.Add(Member(1));
	R.SetGold(500);

	CHECK(R.BuyItem(UnitId{1}, Stock, 0));
	CHECK(R.Gold() == 200);
	CHECK(R.Find(UnitId{1})->Items.size() == 1);
	CHECK(!R.BuyItem(UnitId{1}, Stock, 0)); // 200 gold is not enough
	CHECK(R.Gold() == 200);
	CHECK(!R.BuyItem(UnitId{1}, Stock, 3)); // no such entry
	CHECK(!R.BuyItem(UnitId{9}, Stock, 0)); // unknown member

	CHECK(R.SellItem(UnitId{1}, 0));        // half of 300
	CHECK(R.Gold() == 350);
	CHECK(R.Find(UnitId{1})->Items.empty());
	CHECK(!R.SellItem(UnitId{1}, 0));       // nothing left
	CHECK(!R.SellItem(UnitId{9}, 0));

	// a full pack cannot buy; an unpriced item cannot be sold
	BattleUnit Full = Member(2);
	Full.Items = {Potion(), Potion(), Potion(), Potion(), Potion()};
	R.Add(Full);
	R.SetGold(1000);
	CHECK(!R.BuyItem(UnitId{2}, Stock, 0));
	CHECK(!R.SellItem(UnitId{2}, 0)); // Potion() has Price 0
	CHECK(R.Gold() == 1000);
}

static void TestApplyBattleResultKeepsProgress()
{
	Roster R;
	R.Add(Member(1));
	R.Add(Member(2)); // not deployed
	R.SetGold(100);

	// the battle: unit 1 levelled up, hurt, picked up a potion, moved; the army earned gold
	BattleUnit Fought = Member(1, 3);
	Fought.Experience = 40;
	Fought.Hp = 4;
	Fought.Position = {4, 0};
	Fought.HasMoved = true;
	Fought.HasActed = true;
	Fought.Items = {Potion()};
	Battle B = MakeBattle({Fought, Enemy(10, 5)});
	B.SetGold(250);

	R.ApplyBattleResult(B, {UnitId{1}}, Difficulty::Classic);

	const BattleUnit* After = R.Find(UnitId{1});
	CHECK(After != nullptr);
	CHECK(After->Level == 3 && After->Experience == 40);
	CHECK(After->Items.size() == 1);
	CHECK(After->Hp == After->MaxHp); // healed
	CHECK(!After->HasMoved && !After->HasActed);
	CHECK(R.Find(UnitId{2}) != nullptr); // untouched
	CHECK(R.Gold() == 250);
	CHECK(R.Members().size() == 2);
}

static void TestDeadUnits()
{
	// unit 2 was deployed but is not in the finished battle: it died
	for (const Difficulty D : {Difficulty::Classic, Difficulty::Casual})
	{
		Roster R;
		BattleUnit Dead = Member(2, 4);
		Dead.Experience = 10;
		R.Add(Member(1));
		R.Add(Dead);

		Battle B = MakeBattle({Member(1), Enemy(10, 5)});
		R.ApplyBattleResult(B, {UnitId{1}, UnitId{2}}, D);

		if (D == Difficulty::Classic)
		{
			CHECK(R.Find(UnitId{2}) == nullptr); // lost for good
			CHECK(R.Members().size() == 1);
		}
		else
		{
			const BattleUnit* Back = R.Find(UnitId{2});
			CHECK(Back != nullptr); // returns next chapter, as it was before the battle
			CHECK(Back != nullptr && Back->Level == 4 && Back->Experience == 10);
		}
	}
}

int main()
{
	TestAdd();
	TestGold();
	TestDeploy();
	TestItemOrganizing();
	TestEquipWeapon();
	TestBuyAndSell();
	TestApplyBattleResultKeepsProgress();
	TestDeadUnits();
	return TEST_MAIN_RESULT();
}
