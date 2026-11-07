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

int main()
{
	MoveReturnsUnitMovedEvent();
	RuleViolationsAreRejectedAndStateIsUnchanged();
	WaitAttackAndEndPhaseProduceTheirEvents();
	CommandsAfterTheBattleEndsReportBattleEnded();
	return TEST_MAIN_RESULT();
}
