#include <vector>

#include "Battle/Battle.h"
#include "TestSupport.h"

using namespace commander::domain;

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

static BattleUnit Fighter(std::uint32_t Id, Faction Side, std::int32_t X, std::int32_t Hp)
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, 3};
	U.Hp = U.MaxHp = Hp;
	U.Strength = 5;
	U.Weapon = WeaponSpec{3, 100, 1, 1};
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

// ---- stat caps ----

static void LevelUpDoesNotRaiseAStatPastItsCap()
{
	BattleUnit Hero = Fighter(1, Faction::Player, 0, 30);
	Hero.Experience = 95;
	Hero.Growth = GrowthRates{100, 100, 100, 100, 100};
	Hero.Strength = 6;
	Hero.Skill = 3;
	Hero.Caps = StatCaps{30, 6, 0, 3, 0}; // Hp 30 and Strength 6 are at the cap, Skill uncapped, Speed 0 < 3
	Battle B = MakeBattle({Hero, Fighter(2, Faction::Enemy, 1, 60)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value() && R->LevelUps.size() == 1);
	const LevelUp& L = R->LevelUps[0];
	CHECK(L.HpGain == 0 && L.StrengthGain == 0);
	CHECK(L.SkillGain == 1 && L.SpeedGain == 1 && L.DefenseGain == 1);
}

// ---- promotion ----

static PromotionSpec KnightPromotion()
{
	PromotionSpec P;
	P.Name = "knight";
	P.MinLevel = 10;
	P.Bonus = StatBonus{3, 2, 1, 1, 2, 1};
	P.Growth = GrowthRates{80, 50, 40, 30, 40};
	P.Caps = StatCaps{60, 30, 28, 28, 30};
	return P;
}

static PromotionSpec MagePromotion()
{
	PromotionSpec P;
	P.Name = "mage";
	P.MinLevel = 8;
	P.Bonus = StatBonus{0, 4, 3, 2, 0, 0};
	P.Growth = GrowthRates{40, 60, 50, 40, 10};
	P.Caps = StatCaps{35, 32, 30, 30, 15};
	return P;
}

// A veteran with two class-change options: 0 = knight (level 10), 1 = mage (level 8).
static BattleUnit Veteran(std::int32_t Level)
{
	BattleUnit U = Fighter(1, Faction::Player, 0, 20);
	U.Level = Level;
	U.Experience = 40;
	U.Promotions = {KnightPromotion(), MagePromotion()};
	return U;
}

static ItemSlot Seal(std::int32_t Uses, std::int32_t MinLevel)
{
	ItemSlot S;
	S.Kind = ItemKind::PromotionSeal;
	S.Uses = Uses;
	S.Amount = MinLevel;
	return S;
}

static void PromotionAppliesTheChosenOptionAndResetsLevel()
{
	Battle B = MakeBattle({Veteran(10), Fighter(2, Faction::Enemy, 5, 20)});
	const auto E = B.PromoteUnit(UnitId{1}, 0);
	CHECK(E.has_value() && E->Unit == UnitId{1} && E->Option == 0 && E->Name == "knight" && !E->UsedSeal);
	CHECK(E->Bonus.Hp == 3 && E->Bonus.Movement == 1);
	const BattleUnit* U = B.FindUnit(UnitId{1});
	CHECK(U->Level == 1 && U->Experience == 0);
	CHECK(U->MaxHp == 23 && U->Hp == 23);
	CHECK(U->Strength == 7 && U->Skill == 1 && U->Speed == 1 && U->Defense == 2);
	CHECK(U->Movement == 4);
	CHECK(U->Growth.Hp == 80 && U->Caps.Hp == 60);
	CHECK(U->Promotions.empty()); // all the other options are gone too
	CHECK(U->HasActed && U->HasMoved);
}

static void TheSecondOptionHasItsOwnLevelAndBonuses()
{
	Battle B = MakeBattle({Veteran(8), Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(!B.PromoteUnit(UnitId{1}, 0).has_value()); // knight needs level 10
	const auto E = B.PromoteUnit(UnitId{1}, 1);       // mage needs level 8
	CHECK(E.has_value() && E->Name == "mage" && E->Option == 1);
	const BattleUnit* U = B.FindUnit(UnitId{1});
	CHECK(U->Strength == 9 && U->Skill == 3 && U->Speed == 2 && U->Movement == 3);
	CHECK(U->Growth.Strength == 60 && U->Caps.Defense == 15);
}

static void PromotionRejections()
{
	Battle Low = MakeBattle({Veteran(7), Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(!Low.PromoteUnit(UnitId{1}, 1).has_value()); // level 7 < 8
	CHECK(Low.FindUnit(UnitId{1})->Promotions.size() == 2);

	Battle B = MakeBattle({Veteran(12), Fighter(2, Faction::Enemy, 5, 20), Fighter(3, Faction::Player, 3, 20)});
	CHECK(!B.PromoteUnit(UnitId{3}, 0).has_value()); // no promotion available
	CHECK(!B.PromoteUnit(UnitId{2}, 0).has_value()); // not the enemy's phase
	CHECK(!B.PromoteUnit(UnitId{9}, 0).has_value()); // unknown
	CHECK(!B.PromoteUnit(UnitId{1}, 2).has_value()); // no such option
	CHECK(B.PromoteUnit(UnitId{1}, 0).has_value());
	B.EndPhase();
	B.EndPhase();
	CHECK(!B.PromoteUnit(UnitId{1}, 0).has_value()); // only once

	Battle Acted = MakeBattle({Veteran(12), Fighter(2, Faction::Enemy, 5, 20)});
	Acted.WaitUnit(UnitId{1});
	CHECK(!Acted.PromoteUnit(UnitId{1}, 0).has_value());
}

static void ASealPromotesBelowTheNormalLevel()
{
	BattleUnit U = Veteran(5); // knight needs 10, the seal allows level 5+
	U.Items = {Seal(2, 5)};
	Battle B = MakeBattle({U, Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(!B.PromoteUnit(UnitId{1}, 0).has_value()); // not without the seal
	const auto E = B.PromoteUnit(UnitId{1}, 0, 0);
	CHECK(E.has_value() && E->UsedSeal && E->Name == "knight");
	const BattleUnit* After = B.FindUnit(UnitId{1});
	CHECK(After->Items.size() == 1 && After->Items[0].Uses == 1); // one charge spent
	CHECK(After->Level == 1 && After->Promotions.empty());
}

static void ASpentSealIsRemoved()
{
	BattleUnit U = Veteran(5);
	U.Items = {Seal(1, 5)};
	Battle B = MakeBattle({U, Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(B.PromoteUnit(UnitId{1}, 0, 0).has_value());
	CHECK(B.FindUnit(UnitId{1})->Items.empty());
}

static void SealRejections()
{
	BattleUnit U = Veteran(4);
	U.Items = {Seal(1, 5), Seal(1, 1)};
	Battle B = MakeBattle({U, Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(!B.PromoteUnit(UnitId{1}, 0, 0).has_value());  // level 4 < the seal's level 5
	CHECK(!B.PromoteUnit(UnitId{1}, 0, 9).has_value());  // no such slot
	CHECK(!B.UseItem(UnitId{1}, 0).has_value());         // seals are used by promoting, not by UseItem
	CHECK(B.FindUnit(UnitId{1})->Items.size() == 2);

	BattleUnit V = Veteran(12);
	ItemSlot Potion;
	Potion.Kind = ItemKind::Potion;
	Potion.Uses = 1;
	Potion.Amount = 5;
	V.Items = {Potion};
	Battle C = MakeBattle({V, Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(!C.PromoteUnit(UnitId{1}, 0, 0).has_value()); // slot 0 is not a seal
	BattleUnit NoOptions = Fighter(1, Faction::Player, 0, 20);
	NoOptions.Items = {Seal(1, 1)};
	Battle D = MakeBattle({NoOptions, Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(!D.PromoteUnit(UnitId{1}, 0, 0).has_value()); // nothing to promote into
}

// ---- weapon ranks ----

static BattleUnit Lancer(std::int32_t Points)
{
	BattleUnit U = Fighter(1, Faction::Player, 0, 30);
	U.Weapon = WeaponSpec{6, 100, 1, 1, 0, 1, 1}; // type 1, needs rank 1
	U.WeaponPoints[1] = Points;
	return U;
}

static void RankComesFromPoints()
{
	CHECK(WeaponRankOf(0) == 0);
	CHECK(WeaponRankOf(RankThreshold[1] - 1) == 0);
	CHECK(WeaponRankOf(RankThreshold[1]) == 1);
	CHECK(WeaponRankOf(RankThreshold[2]) == 2);
	CHECK(WeaponRankOf(RankThreshold[MaxWeaponRank]) == MaxWeaponRank);
	CHECK(WeaponRankOf(100000) == MaxWeaponRank);
}

static void AWeaponAboveTheUnitsRankCannotBeUsed()
{
	Battle B = MakeBattle({Lancer(0), Fighter(2, Faction::Enemy, 1, 30)});
	FixedRandom Rng({0});
	CHECK(!HasUsableWeapon(*B.FindUnit(UnitId{1})));
	CHECK(!B.AttackUnit(UnitId{1}, UnitId{2}, Rng).has_value());
	Battle Skilled = MakeBattle({Lancer(RankThreshold[1]), Fighter(2, Faction::Enemy, 1, 30)});
	CHECK(Skilled.AttackUnit(UnitId{1}, UnitId{2}, Rng).has_value());
}

static void UntypedWeaponsNeedNoRank()
{
	BattleUnit U = Fighter(1, Faction::Player, 0, 30);
	U.Weapon = WeaponSpec{3, 100, 1, 1, 0, 0, 3}; // type 0 ignores the required rank
	CHECK(HasUsableWeapon(U));
}

static void EquippingNeedsTheRank()
{
	ItemSlot Spare;
	Spare.Kind = ItemKind::Weapon;
	Spare.Weapon = WeaponSpec{6, 100, 1, 1, 0, 1, 2}; // type 1, rank 2
	BattleUnit U = Fighter(1, Faction::Player, 0, 30);
	U.Items = {Spare};
	U.WeaponPoints[1] = RankThreshold[1];
	Battle B = MakeBattle({U, Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(!B.EquipWeapon(UnitId{1}, 0).has_value()); // rank 1 < 2
	BattleUnit Better = U;
	Better.WeaponPoints[1] = RankThreshold[2];
	Battle C = MakeBattle({Better, Fighter(2, Faction::Enemy, 5, 20)});
	CHECK(C.EquipWeapon(UnitId{1}, 0).has_value());
}

static void StrikesTrainThePlayersWeaponType()
{
	Battle B = MakeBattle({Lancer(RankThreshold[1] + 3), Fighter(2, Faction::Enemy, 1, 60)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value() && !R->Strikes[0].WeaponRankUp);
	CHECK(B.FindUnit(UnitId{1})->WeaponPoints[1] == RankThreshold[1] + 4);
}

static void ReachingAThresholdReportsARankUp()
{
	FixedRandom Rng({0});
	// One strike short of rank 2: the next strike crosses the threshold.
	Battle C = MakeBattle({Lancer(RankThreshold[2] - 1), Fighter(2, Faction::Enemy, 1, 60)});
	const auto R = C.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value() && R->Strikes[0].WeaponRankUp);
	CHECK(WeaponRankOf(C.FindUnit(UnitId{1})->WeaponPoints[1]) == 2);
}

static void EnemiesAndUntypedWeaponsDoNotTrain()
{
	BattleUnit Foe = Fighter(2, Faction::Enemy, 1, 60);
	Foe.Weapon = WeaponSpec{6, 100, 1, 1, 0, 1, 0};
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 30), Foe});
	FixedRandom Rng({0});
	B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(B.FindUnit(UnitId{2})->WeaponPoints[1] == 0);  // the enemy countered with a typed weapon
	for (const std::int32_t P : B.FindUnit(UnitId{1})->WeaponPoints)
	{
		CHECK(P == 0); // the player's sword is untyped
	}
}

static void AddUnitRejectsInconsistentNewFields()
{
	Battle B = MakeBattle({});
	BattleUnit U = Fighter(1, Faction::Player, 0, 10);
	U.Weapon.Type = MaxWeaponTypes + 1;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Weapon.RequiredRank = MaxWeaponRank + 1;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.WeaponPoints[2] = -1;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Caps.Speed = -1;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Promotions = {KnightPromotion()};
	U.Promotions[0].MinLevel = 0;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Promotions = {KnightPromotion()};
	U.Promotions[0].Growth.Hp = 101;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Promotions = {KnightPromotion()};
	U.Promotions[0].Bonus.Strength = -1;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Promotions = std::vector<PromotionSpec>(MaxPromotionOptions + 1, KnightPromotion());
	CHECK(!B.AddUnit(U)); // too many options
	U = Fighter(1, Faction::Player, 0, 10);
	U.Items = {Seal(1, 0)};
	CHECK(!B.AddUnit(U)); // a seal needs a level of at least 1
	U = Fighter(1, Faction::Player, 0, 10);
	U.Items = {Seal(1, MaxLevel + 1)};
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Items = {Seal(0, 5)};
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Promotions = {KnightPromotion(), MagePromotion()};
	U.Items = {Seal(1, 5)};
	CHECK(B.AddUnit(U));
}

int main()
{
	LevelUpDoesNotRaiseAStatPastItsCap();
	PromotionAppliesTheChosenOptionAndResetsLevel();
	TheSecondOptionHasItsOwnLevelAndBonuses();
	PromotionRejections();
	ASealPromotesBelowTheNormalLevel();
	ASpentSealIsRemoved();
	SealRejections();
	RankComesFromPoints();
	AWeaponAboveTheUnitsRankCannotBeUsed();
	UntypedWeaponsNeedNoRank();
	EquippingNeedsTheRank();
	StrikesTrainThePlayersWeaponType();
	ReachingAThresholdReportsARankUp();
	EnemiesAndUntypedWeaponsDoNotTrain();
	AddUnitRejectsInconsistentNewFields();
	return TEST_MAIN_RESULT();
}
