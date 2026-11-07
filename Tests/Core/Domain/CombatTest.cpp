#include <vector>

#include "Battle/Battle.h"
#include "TestSupport.h"

using namespace commander::domain;

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

static BattleUnit Fighter(std::uint32_t Id, Faction Side, std::int32_t X, std::int32_t Hp, std::int32_t Strength,
	std::int32_t Defense, WeaponSpec Weapon)
{
	BattleUnit U{UnitId{Id}, Side, {X, 0}, 3};
	U.Hp = Hp;
	U.MaxHp = Hp;
	U.Strength = Strength;
	U.Defense = Defense;
	U.Weapon = Weapon;
	return U;
}

static constexpr WeaponSpec Sword{3, 100, 1, 1};
static constexpr WeaponSpec Bow{2, 100, 2, 2};
static constexpr WeaponSpec NoWeapon{0, 0, 0, 0};

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

static void DamageIsStrengthPlusMightMinusDefenseFlooredAtZero()
{
	BattleUnit A = Fighter(1, Faction::Player, 0, 10, 5, 0, Sword);
	BattleUnit D = Fighter(2, Faction::Enemy, 1, 10, 0, 2, Sword);
	CHECK(CalculateDamage(A, D) == 6);
	D.Defense = 20;
	CHECK(CalculateDamage(A, D) == 0);
}

static void HitRateUsesSkillAndSpeedAndIsClamped()
{
	BattleUnit A = Fighter(1, Faction::Player, 0, 10, 5, 0, WeaponSpec{3, 70, 1, 1});
	BattleUnit D = Fighter(2, Faction::Enemy, 1, 10, 0, 0, Sword);
	CHECK(CalculateHitRate(A, D) == 70);
	A.Skill = 5;
	D.Speed = 3;
	CHECK(CalculateHitRate(A, D) == 74); // 70 + 10 - 6
	D.Speed = 100;
	CHECK(CalculateHitRate(A, D) == 0);
	A.Skill = 100;
	D.Speed = 0;
	CHECK(CalculateHitRate(A, D) == 100);
}

static void AttackHitsThenTargetCountersInRange()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 10, 5, 1, Sword), Fighter(2, Faction::Enemy, 1, 10, 4, 2, Sword)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Strikes.size() == 2);
	CHECK(R->Strikes[0].Attacker == UnitId{1});
	CHECK(R->Strikes[0].Hit && R->Strikes[0].Damage == 6 && R->Strikes[0].DefenderHp == 4);
	CHECK(R->Strikes[1].Attacker == UnitId{2});
	CHECK(R->Strikes[1].Hit && R->Strikes[1].Damage == 6 && R->Strikes[1].DefenderHp == 4);
	CHECK(B.FindUnit(UnitId{2})->Hp == 4);
	CHECK(B.FindUnit(UnitId{1})->Hp == 4);
	CHECK(R->Defeated.empty());
}

static void MissDealsNoDamage()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 10, 5, 0, WeaponSpec{3, 50, 1, 1}),
		Fighter(2, Faction::Enemy, 1, 10, 4, 0, NoWeapon)});
	FixedRandom Rng({50}); // roll 50 vs hit rate 50: miss (hit only when roll < rate)
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Strikes.size() == 1);
	CHECK(!R->Strikes[0].Hit && R->Strikes[0].Damage == 0 && R->Strikes[0].DefenderHp == 10);
	CHECK(B.FindUnit(UnitId{2})->Hp == 10);
}

static void KillingBlowPreventsCounterAndRemovesUnit()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 10, 5, 0, Sword), Fighter(2, Faction::Enemy, 1, 6, 4, 0, Sword)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Strikes.size() == 1);
	CHECK(R->Defeated.size() == 1 && R->Defeated[0] == UnitId{2});
	CHECK(B.FindUnit(UnitId{2}) == nullptr);
	CHECK(B.UnitAt({1, 0}) == nullptr);
	CHECK(B.FindUnit(UnitId{1})->Hp == 10);
}

static void CounterCanDefeatTheAttacker()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 5, 0, 0, Sword), Fighter(2, Faction::Enemy, 1, 20, 4, 0, Sword)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Strikes.size() == 2);
	CHECK(R->Defeated.size() == 1 && R->Defeated[0] == UnitId{1});
	CHECK(B.FindUnit(UnitId{1}) == nullptr);
}

static void NoCounterWhenAttackerIsOutOfTargetsRange()
{
	// Archer (range 2) attacks a swordsman (range 1) from distance 2: no counter.
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 10, 5, 0, Bow), Fighter(2, Faction::Enemy, 2, 10, 4, 0, Sword)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(R->Strikes.size() == 1);
	CHECK(B.FindUnit(UnitId{1})->Hp == 10);
}

static void AttackRejectsOutOfRange()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 10, 5, 0, Sword), Fighter(2, Faction::Enemy, 2, 10, 4, 0, Sword),
		Fighter(3, Faction::Enemy, 1, 10, 4, 0, NoWeapon)});
	FixedRandom Rng({0});
	CHECK(!B.AttackUnit(UnitId{1}, UnitId{2}, Rng).has_value()); // distance 2, sword range 1
	// Bow cannot hit an adjacent target.
	Battle C = MakeBattle({Fighter(1, Faction::Player, 0, 10, 5, 0, Bow), Fighter(2, Faction::Enemy, 1, 10, 4, 0, Sword)});
	CHECK(!C.AttackUnit(UnitId{1}, UnitId{2}, Rng).has_value());
	CHECK(C.FindUnit(UnitId{2})->Hp == 10);
}

static void AttackRejectsInvalidParticipants()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 10, 5, 0, Sword), Fighter(2, Faction::Enemy, 1, 10, 4, 0, Sword),
		Fighter(3, Faction::Player, 2, 10, 4, 0, Sword), Fighter(4, Faction::Player, 3, 10, 4, 0, NoWeapon)});
	FixedRandom Rng({0});
	CHECK(!B.AttackUnit(UnitId{1}, UnitId{3}, Rng).has_value()); // same faction
	CHECK(!B.AttackUnit(UnitId{2}, UnitId{1}, Rng).has_value()); // not the current phase's faction
	CHECK(!B.AttackUnit(UnitId{9}, UnitId{2}, Rng).has_value()); // unknown attacker
	CHECK(!B.AttackUnit(UnitId{1}, UnitId{9}, Rng).has_value()); // unknown target
	CHECK(!B.AttackUnit(UnitId{4}, UnitId{2}, Rng).has_value()); // no weapon
	CHECK(!B.AttackUnit(UnitId{1}, UnitId{1}, Rng).has_value()); // self
	CHECK(B.FindUnit(UnitId{2})->Hp == 10);
}

static void AttackEndsTheUnitsActionAndMayFollowAMove()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 10, 5, 0, Sword), Fighter(2, Faction::Enemy, 3, 20, 4, 0, Sword)});
	FixedRandom Rng({0});
	CHECK(B.MoveUnit(UnitId{1}, {2, 0}).has_value());
	CHECK(B.AttackUnit(UnitId{1}, UnitId{2}, Rng).has_value());
	CHECK(B.FindUnit(UnitId{1})->HasActed);
	CHECK(!B.AttackUnit(UnitId{1}, UnitId{2}, Rng).has_value());
	CHECK(!B.WaitUnit(UnitId{1}).has_value());
}

static void AddUnitRejectsNonPositiveHp()
{
	Battle B = MakeBattle({});
	CHECK(!B.AddUnit(Fighter(1, Faction::Player, 0, 0, 5, 0, Sword)));
}

int main()
{
	DamageIsStrengthPlusMightMinusDefenseFlooredAtZero();
	HitRateUsesSkillAndSpeedAndIsClamped();
	AttackHitsThenTargetCountersInRange();
	MissDealsNoDamage();
	KillingBlowPreventsCounterAndRemovesUnit();
	CounterCanDefeatTheAttacker();
	NoCounterWhenAttackerIsOutOfTargetsRange();
	AttackRejectsOutOfRange();
	AttackRejectsInvalidParticipants();
	AttackEndsTheUnitsActionAndMayFollowAMove();
	AddUnitRejectsNonPositiveHp();
	return TEST_MAIN_RESULT();
}
