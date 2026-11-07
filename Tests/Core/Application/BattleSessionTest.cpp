#include <variant>
#include <vector>

#include "Battle/BattleSession.h"
#include "TestSupport.h"

using namespace commander::application;
using namespace commander::domain;

class AlwaysHit : public IRandomSource
{
public:
	std::int32_t NextPercent() override { return 0; }
};

static BattleUnit Fighter(std::uint32_t Id, Faction Side, std::int32_t X, std::int32_t Hp)
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, 3};
	U.Hp = U.MaxHp = Hp;
	U.Strength = 5;
	U.Weapon = WeaponSpec{3, 100, 1, 1}; // 8 damage
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

static void MoveReturnsUnitMovedEvent()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 10), Fighter(2, Faction::Enemy, 5, 10)}), Rng);
	const auto R = S.Execute(MoveUnitCommand{UnitId{1}, {2, 0}});
	CHECK(R.IsOk());
	const auto* Moved = std::get_if<UnitMoved>(&R.Value());
	CHECK(Moved != nullptr);
	CHECK(Moved != nullptr && Moved->Path.back() == (GridCoord{2, 0}));
	CHECK(S.GetBattle().FindUnit(UnitId{1})->Position == (GridCoord{2, 0}));
}

static void RuleViolationsAreRejectedAndStateIsUnchanged()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 10), Fighter(2, Faction::Enemy, 5, 10)}), Rng);
	const auto R = S.Execute(MoveUnitCommand{UnitId{2}, {4, 0}}); // enemy unit during player phase
	CHECK(!R.IsOk() && R.Error() == CommandError::Rejected);
	CHECK(S.GetBattle().FindUnit(UnitId{2})->Position == (GridCoord{5, 0}));
	CHECK(S.Execute(WaitUnitCommand{UnitId{9}}).Error() == CommandError::Rejected);
	CHECK(S.Execute(AttackUnitCommand{UnitId{1}, UnitId{2}}).Error() == CommandError::Rejected); // out of range
}

static void WaitAttackAndEndPhaseProduceTheirEvents()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 1, 20),
		Fighter(3, Faction::Player, 4, 20)}), Rng);
	const auto Attack = S.Execute(AttackUnitCommand{UnitId{1}, UnitId{2}});
	CHECK(Attack.IsOk() && std::get_if<AttackResolved>(&Attack.Value()) != nullptr);
	const auto Wait = S.Execute(WaitUnitCommand{UnitId{3}});
	CHECK(Wait.IsOk() && std::get_if<UnitWaited>(&Wait.Value()) != nullptr);
	const auto End = S.Execute(EndPhaseCommand{});
	CHECK(End.IsOk());
	const auto* Phase = std::get_if<PhaseChanged>(&End.Value());
	CHECK(Phase != nullptr && Phase->Side == Faction::Enemy);
}

static void CommandsAfterTheBattleEndsReportBattleEnded()
{
	AlwaysHit Rng;
	BattleSession S(MakeBattle({Fighter(1, Faction::Player, 0, 20), Fighter(2, Faction::Enemy, 1, 5),
		Fighter(3, Faction::Player, 4, 20)}), Rng);
	const auto Attack = S.Execute(AttackUnitCommand{UnitId{1}, UnitId{2}});
	CHECK(Attack.IsOk());
	CHECK(S.GetBattle().Outcome() == BattleOutcome::PlayerWon);
	CHECK(S.Execute(WaitUnitCommand{UnitId{3}}).Error() == CommandError::BattleEnded);
	CHECK(S.Execute(EndPhaseCommand{}).Error() == CommandError::BattleEnded);
}

static void EquipAndUseItemProduceTheirEvents()
{
	AlwaysHit Rng;
	BattleUnit Hero = Fighter(1, Faction::Player, 0, 10);
	Hero.MaxHp = 20;
	ItemSlot Spare;
	Spare.Kind = ItemKind::Weapon;
	Spare.Weapon = WeaponSpec{6, 100, 1, 1, 0};
	ItemSlot Potion;
	Potion.Kind = ItemKind::Potion;
	Potion.Uses = 1;
	Potion.Amount = 8;
	Hero.Items = {Spare, Potion};
	BattleSession S(MakeBattle({Hero, Fighter(2, Faction::Enemy, 5, 10)}), Rng);

	const auto Equip = S.Execute(EquipWeaponCommand{UnitId{1}, 0});
	CHECK(Equip.IsOk() && std::get_if<WeaponEquipped>(&Equip.Value()) != nullptr);
	CHECK(S.GetBattle().FindUnit(UnitId{1})->Weapon.Might == 6);

	const auto Use = S.Execute(UseItemCommand{UnitId{1}, 1});
	CHECK(Use.IsOk());
	const auto* Used = std::get_if<ItemUsed>(&Use.Value());
	CHECK(Used != nullptr && Used->HpHealed == 8);

	CHECK(S.Execute(UseItemCommand{UnitId{1}, 0}).Error() == CommandError::Rejected); // already acted
}

static void TradeStaffShopAndPromotionCommandsProduceTheirEvents()
{
	AlwaysHit Rng;
	ItemSlot Potion;
	Potion.Kind = ItemKind::Potion;
	Potion.Uses = 1;
	Potion.Amount = 5;
	Potion.Price = 20;
	ItemSlot Staff;
	Staff.Kind = ItemKind::Staff;
	Staff.Uses = 2;
	Staff.Amount = 6;
	Staff.Range = 2;

	BattleUnit Healer = Fighter(1, Faction::Player, 0, 20);
	Healer.Items = {Staff, Potion};
	BattleUnit Wounded = Fighter(2, Faction::Player, 1, 20);
	Wounded.Hp = 4;
	BattleUnit Veteran = Fighter(3, Faction::Player, 3, 20);
	Veteran.Level = 5;
	Veteran.Promotions = {PromotionSpec{"knight", 5, StatBonus{1, 1, 0, 0, 0, 1}, GrowthRates{}, StatCaps{}},
		PromotionSpec{"mage", 8, StatBonus{0, 3, 0, 0, 0, 0}, GrowthRates{}, StatCaps{}}};
	Battle B = MakeBattle({Healer, Wounded, Veteran, Fighter(9, Faction::Enemy, 5, 20)});
	Shop S;
	S.Position = {3, 0};
	ItemSlot ForSale = Potion;
	ForSale.Price = 30;
	S.Stock = {ForSale};
	B.AddShop(S);
	B.SetGold(50);
	BattleSession Session(std::move(B), Rng);

	const auto Buy = Session.Execute(BuyItemCommand{UnitId{3}, 0});
	CHECK(Buy.IsOk() && std::get_if<ItemBought>(&Buy.Value()) != nullptr);
	const auto Sell = Session.Execute(SellItemCommand{UnitId{3}, 0});
	const auto* Sold = Sell.IsOk() ? std::get_if<ItemSold>(&Sell.Value()) : nullptr;
	CHECK(Sold != nullptr && Sold->Gold == 15);
	CHECK(Session.Execute(PromoteUnitCommand{UnitId{3}, 1, std::nullopt}).Error() == CommandError::Rejected); // mage needs level 8
	const auto Promote = Session.Execute(PromoteUnitCommand{UnitId{3}, 0, std::nullopt});
	const auto* Promoted = Promote.IsOk() ? std::get_if<UnitPromoted>(&Promote.Value()) : nullptr;
	CHECK(Promoted != nullptr && Promoted->Name == "knight" && !Promoted->UsedSeal);

	const auto Heal = Session.Execute(UseStaffCommand{UnitId{1}, UnitId{2}, 0});
	const auto* Healed = Heal.IsOk() ? std::get_if<StaffUsed>(&Heal.Value()) : nullptr;
	CHECK(Healed != nullptr && Healed->HpHealed == 6);
	CHECK(Healed != nullptr && Healed->Experience.has_value() && Healed->Experience->Amount == 11); // 8 + 6 / 2

	Session.Execute(EndPhaseCommand{});
	Session.Execute(EndPhaseCommand{});
	const auto Give = Session.Execute(GiveItemCommand{UnitId{1}, UnitId{2}, 1});
	CHECK(Give.IsOk() && std::get_if<ItemGiven>(&Give.Value()) != nullptr);
	const auto Drop = Session.Execute(DropItemCommand{UnitId{2}, 0});
	CHECK(Drop.IsOk() && std::get_if<ItemDropped>(&Drop.Value()) != nullptr);
	CHECK(Session.Execute(DropItemCommand{UnitId{2}, 0}).Error() == CommandError::Rejected); // nothing left to drop
}

int main()
{
	TradeStaffShopAndPromotionCommandsProduceTheirEvents();
	EquipAndUseItemProduceTheirEvents();
	MoveReturnsUnitMovedEvent();
	RuleViolationsAreRejectedAndStateIsUnchanged();
	WaitAttackAndEndPhaseProduceTheirEvents();
	CommandsAfterTheBattleEndsReportBattleEnded();
	return TEST_MAIN_RESULT();
}
