#include <vector>

#include "Battle/Campaign.h"
#include "TestSupport.h"

using namespace commander::application;
using namespace commander::domain;

class ZeroRandom : public IRandomSource
{
public:
	std::int32_t NextPercent() override { return 0; }
};

static BattleUnit Member(std::uint32_t Id)
{
	BattleUnit U{UnitId{Id}, Faction::Player, {0, 0}, 3};
	U.Hp = 20;
	U.MaxHp = 20;
	U.Weapon.Might = 5;
	U.Weapon.Hit = 100;
	U.Weapon.MinRange = 1;
	U.Weapon.MaxRange = 1;
	return U;
}

// 4x1 map. A fragile boss at x=3; the player's start cells are x=2 and x=1, so the first sortie member is adjacent.
static Chapter MakeChapter(const char* Name)
{
	Chapter C;
	C.Name = Name;
	C.Setup.Map.Width = 4;
	C.Setup.Map.Height = 1;
	C.Setup.Map.Terrains = {{1, true}, {0, false}};
	C.Setup.Map.Cells = {0, 0, 0, 0};
	BattleUnit Boss{UnitId{100}, Faction::Enemy, {3, 0}, 3};
	Boss.IsBoss = true;
	Boss.Hp = 1;
	Boss.MaxHp = 1;
	C.Setup.Units = {Boss};
	C.Setup.Conditions.DefeatBoss = true;
	C.Setup.Conditions.TurnLimit = 1;
	C.StartCells = {GridCoord{2, 0}, GridCoord{1, 0}};
	return C;
}

static Roster MakeRoster()
{
	Roster R;
	R.Add(Member(1));
	R.Add(Member(2));
	return R;
}

static Campaign Make(Difficulty D = Difficulty::Classic)
{
	return Campaign(MakeRoster(), D, {MakeChapter("one"), MakeChapter("two")});
}

// Deploys unit 1 and plays the prepared battle to victory.
static Battle WinBattle(Campaign& C, std::vector<UnitId>& Deployed)
{
	C.Preparation().Deploy(UnitId{1});
	auto Started = C.Preparation().Start();
	Battle B = Started.Value().Battle;
	Deployed = Started.Value().Deployed;
	ZeroRandom Rng;
	B.AttackUnit(UnitId{1}, UnitId{100}, Rng);
	return B;
}

static void StartsAtFirstChapter()
{
	Campaign C = Make();
	CHECK(!C.IsFinished());
	CHECK(C.ChapterIndex() == 0);
	CHECK(C.CurrentChapter().Name == "one");
	CHECK(C.Preparation().MaxDeployed() == 2);
	CHECK(C.Roster().Members().size() == 2);
}

static void EmptyCampaignIsFinished()
{
	Campaign C(MakeRoster(), Difficulty::Classic, {});
	CHECK(C.IsFinished());
}

static void PreparationIsKeptBetweenCalls()
{
	Campaign C = Make();
	C.Preparation().Deploy(UnitId{1});
	CHECK(C.Preparation().Deployed().size() == 1);
}

static void WinAdvancesToTheNextChapter()
{
	Campaign C = Make();
	std::vector<UnitId> Deployed;
	const Battle B = WinBattle(C, Deployed);
	CHECK(B.Outcome() == BattleOutcome::PlayerWon);

	CHECK(C.Complete(B, Deployed) == CampaignStep::NextChapter);
	CHECK(C.ChapterIndex() == 1);
	CHECK(C.CurrentChapter().Name == "two");
	CHECK(C.Preparation().Deployed().empty()); // a fresh preparation
}

static void WinningTheLastChapterClearsTheCampaign()
{
	Campaign C = Make();
	std::vector<UnitId> Deployed;
	CHECK(C.Complete(WinBattle(C, Deployed), Deployed) == CampaignStep::NextChapter);
	CHECK(C.Complete(WinBattle(C, Deployed), Deployed) == CampaignStep::CampaignCleared);
	CHECK(C.IsFinished());
}

static void LossRetriesTheSameChapterWithTheRosterUnchanged()
{
	Campaign C = Make();
	C.Preparation().Deploy(UnitId{1});
	auto Started = C.Preparation().Start();
	Battle B = Started.Value().Battle;
	B.EndPhase();
	B.EndPhase(); // turn limit of 1 -> lost
	CHECK(B.Outcome() == BattleOutcome::PlayerLost);

	CHECK(C.Complete(B, Started.Value().Deployed) == CampaignStep::Retry);
	CHECK(C.ChapterIndex() == 0);
	CHECK(C.Roster().Members().size() == 2);
}

static void OngoingBattleIsRefused()
{
	Campaign C = Make();
	C.Preparation().Deploy(UnitId{1});
	auto Started = C.Preparation().Start();
	CHECK(C.Complete(Started.Value().Battle, Started.Value().Deployed) == CampaignStep::BattleOngoing);
	CHECK(C.ChapterIndex() == 0);
}

static void WinCarriesProgressIntoTheRoster()
{
	Campaign C = Make();
	std::vector<UnitId> Deployed;
	Battle B = WinBattle(C, Deployed);
	B.SetGold(300);
	C.Complete(B, Deployed);
	CHECK(C.Roster().Gold() == 300);
	CHECK(C.Roster().Find(UnitId{1}) != nullptr);
	CHECK(C.Roster().Find(UnitId{1})->Experience > 0); // the kill earned experience
}

static void ItemChangesInPreparationSurviveAFinishedBattle()
{
	ItemSlot Potion;
	Potion.Kind = ItemKind::Potion;
	Potion.Uses = 1;
	Potion.Amount = 5;
	BattleUnit A = Member(1);
	A.Items = {Potion};
	Roster R;
	R.Add(A);
	R.Add(Member(2));
	Campaign C2(R, Difficulty::Casual, {MakeChapter("one")});
	C2.Preparation().TransferItem(UnitId{1}, UnitId{2}, 0);
	C2.Preparation().Deploy(UnitId{2});
	auto Started = C2.Preparation().Start();
	CHECK(Started.IsOk());
	Battle B = Started.Value().Battle;
	ZeroRandom Rng;
	B.AttackUnit(UnitId{2}, UnitId{100}, Rng);
	C2.Complete(B, Started.Value().Deployed);
	CHECK(C2.Roster().Find(UnitId{2})->Items.size() == 1);
	CHECK(C2.Roster().Find(UnitId{1})->Items.empty());
}

int main()
{
	StartsAtFirstChapter();
	EmptyCampaignIsFinished();
	PreparationIsKeptBetweenCalls();
	WinAdvancesToTheNextChapter();
	WinningTheLastChapterClearsTheCampaign();
	LossRetriesTheSameChapterWithTheRosterUnchanged();
	OngoingBattleIsRefused();
	WinCarriesProgressIntoTheRoster();
	ItemChangesInPreparationSurviveAFinishedBattle();
	return TEST_MAIN_RESULT();
}
