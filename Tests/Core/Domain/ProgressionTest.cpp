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

// Sword, 8 damage against Defense 0.
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

static const ExperienceGained* GainOf(const AttackResolved& R, std::uint32_t Id)
{
	for (const ExperienceGained& E : R.Experience)
	{
		if (E.Unit.Value == Id)
		{
			return &E;
		}
	}
	return nullptr;
}

static void ExperienceFormula()
{
	BattleUnit Self = Fighter(1, Faction::Player, 0, 10);
	BattleUnit Foe = Fighter(2, Faction::Enemy, 1, 10);
	CHECK(CalculateExperience(Self, Foe, false) == 10);
	CHECK(CalculateExperience(Self, Foe, true) == 30);
	Foe.Level = 4;
	CHECK(CalculateExperience(Self, Foe, false) == 19); // 10 + 3 * 3
	Self.Level = 6;
	CHECK(CalculateExperience(Self, Foe, false) == 10); // a weaker foe adds nothing
}

static void SurvivingPlayerAttackerGainsExperience()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 30), Fighter(2, Faction::Enemy, 1, 30)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value());
	CHECK(GainOf(*R, 1) != nullptr && GainOf(*R, 1)->Amount == 10);
	CHECK(B.FindUnit(UnitId{1})->Experience == 10);
}

static void AKillAddsABonus()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 30), Fighter(2, Faction::Enemy, 1, 5)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value() && GainOf(*R, 1) != nullptr && GainOf(*R, 1)->Amount == 30);
}

static void EnemiesNeverGainExperience()
{
	// The enemy attacks the player; it never gets experience, the countering player does.
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 30), Fighter(2, Faction::Enemy, 1, 30)});
	B.EndPhase();
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{2}, UnitId{1}, Rng);
	CHECK(R.has_value());
	CHECK(GainOf(*R, 2) == nullptr);
	CHECK(GainOf(*R, 1) != nullptr && GainOf(*R, 1)->Amount == 10);
	CHECK(B.FindUnit(UnitId{2})->Experience == 0);
}

static void ADefenderWhoCannotCounterGainsNothing()
{
	BattleUnit Archer = Fighter(1, Faction::Player, 0, 30);
	Archer.Weapon = WeaponSpec{3, 100, 2, 2};
	Battle B = MakeBattle({Archer, Fighter(2, Faction::Enemy, 1, 30)});
	B.EndPhase();
	FixedRandom Rng({0});
	// The sword-wielding enemy at distance 1 cannot be answered by the archer (range 2 only).
	const auto R = B.AttackUnit(UnitId{2}, UnitId{1}, Rng);
	CHECK(R.has_value() && R->Strikes.size() == 1);
	CHECK(R->Experience.empty());
}

static void ADefeatedPlayerGainsNothing()
{
	Battle B = MakeBattle({Fighter(1, Faction::Player, 0, 5), Fighter(2, Faction::Enemy, 1, 30), Fighter(3, Faction::Player, 5, 30)});
	B.EndPhase();
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{2}, UnitId{1}, Rng);
	CHECK(R.has_value() && R->Defeated.size() == 1);
	CHECK(R->Experience.empty());
}

static void CrossingOneHundredLevelsUpAndRollsGrowth()
{
	BattleUnit Hero = Fighter(1, Faction::Player, 0, 30);
	Hero.Experience = 95;
	Hero.Growth = GrowthRates{50, 50, 50, 50, 50};
	Battle B = MakeBattle({Hero, Fighter(2, Faction::Enemy, 1, 30)});
	// Strike roll 0 (hit), counter roll 0 (hit), then growth rolls: Hp yes, Str no, Skill yes, Speed no, Def yes.
	FixedRandom Rng({0, 0, 0, 99, 0, 99, 0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value() && R->LevelUps.size() == 1);
	const LevelUp& L = R->LevelUps[0];
	CHECK(L.Unit == UnitId{1} && L.NewLevel == 2);
	CHECK(L.HpGain == 1 && L.StrengthGain == 0 && L.SkillGain == 1 && L.SpeedGain == 0 && L.DefenseGain == 1);
	const BattleUnit* After = B.FindUnit(UnitId{1});
	CHECK(After->Level == 2 && After->Experience == 5); // 95 + 10 - 100
	CHECK(After->MaxHp == 31 && After->Hp == 22 + 1);   // took an 8 damage counter, then +1 from the level-up
	CHECK(After->Skill == 1 && After->Defense == 1 && After->Strength == 5 && After->Speed == 0);
}

static void ZeroGrowthNeverGains()
{
	BattleUnit Hero = Fighter(1, Faction::Player, 0, 30);
	Hero.Experience = 95;
	Battle B = MakeBattle({Hero, Fighter(2, Faction::Enemy, 1, 30)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value() && R->LevelUps.size() == 1);
	CHECK(R->LevelUps[0].HpGain == 0 && R->LevelUps[0].DefenseGain == 0);
}

static void MaxLevelStopsExperience()
{
	BattleUnit Hero = Fighter(1, Faction::Player, 0, 30);
	Hero.Level = MaxLevel;
	Battle B = MakeBattle({Hero, Fighter(2, Faction::Enemy, 1, 30)});
	FixedRandom Rng({0});
	const auto R = B.AttackUnit(UnitId{1}, UnitId{2}, Rng);
	CHECK(R.has_value() && R->Experience.empty() && R->LevelUps.empty());
	CHECK(B.FindUnit(UnitId{1})->Experience == 0);
}

static void AddUnitRejectsInconsistentProgression()
{
	Battle B = MakeBattle({});
	BattleUnit U = Fighter(1, Faction::Player, 0, 10);
	U.Level = 0;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Level = MaxLevel + 1;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Experience = 100;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Experience = -1;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Growth.Speed = 101;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Growth.Hp = -1;
	CHECK(!B.AddUnit(U));
	U = Fighter(1, Faction::Player, 0, 10);
	U.Level = MaxLevel;
	CHECK(B.AddUnit(U));
}

int main()
{
	ExperienceFormula();
	SurvivingPlayerAttackerGainsExperience();
	AKillAddsABonus();
	EnemiesNeverGainExperience();
	ADefenderWhoCannotCounterGainsNothing();
	ADefeatedPlayerGainsNothing();
	CrossingOneHundredLevelsUpAndRollsGrowth();
	ZeroGrowthNeverGains();
	MaxLevelStopsExperience();
	AddUnitRejectsInconsistentProgression();
	return TEST_MAIN_RESULT();
}
