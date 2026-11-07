// Presentation test: the battle controller drives a whole battle without a World.

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#include "Presentation/CommanderBattleController.h"
#include "Presentation/CommanderCampaignSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderBattleControllerTest,
	"Commander3D.Presentation.BattleController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderBattleControllerTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UCommanderCampaignSubsystem* Campaign = NewObject<UCommanderCampaignSubsystem>(GameInstance);
	UCommanderBattleController* Controller = NewObject<UCommanderBattleController>(GameInstance);

	// Nothing started: harmless no-ops.
	TestFalse(TEXT("No battle yet"), Controller->IsBattleRunning());
	TestTrue(TEXT("No units"), Controller->GetUnits().IsEmpty());
	TestFalse(TEXT("Move needs a battle"), Controller->MoveUnit(1, 0, 0));
	TestFalse(TEXT("End phase needs a battle"), Controller->EndPlayerPhase());
	TestFalse(TEXT("Null campaign"), Controller->BeginBattle(nullptr));

	if (!TestTrue(TEXT("Sample campaign starts"), Campaign->StartCampaign(TEXT("test_campaign"), true)))
	{
		return false;
	}
	TestFalse(TEXT("Nobody deployed: no battle"), Controller->BeginBattle(Campaign));
	Campaign->Deploy(1);
	Campaign->Deploy(2);
	if (!TestTrue(TEXT("Battle begins"), Controller->BeginBattle(Campaign)))
	{
		return false;
	}
	TestFalse(TEXT("A running battle cannot be restarted"), Controller->BeginBattle(Campaign));

	// The starting picture.
	TestTrue(TEXT("Map size"), Controller->GetMapSize() == FIntPoint(5, 4));
	TestEqual(TEXT("Wall cell"), Controller->GetCellMoveCost(2, 1), -1);
	TestEqual(TEXT("Plain cell"), Controller->GetCellMoveCost(0, 0), 1);
	TestEqual(TEXT("Costly cell"), Controller->GetCellMoveCost(2, 0), 2);
	TestEqual(TEXT("Outside cell"), Controller->GetCellMoveCost(9, 9), -1);
	TestEqual(TEXT("Four units"), Controller->GetUnits().Num(), 4);
	TestEqual(TEXT("Two chests"), Controller->GetChests().Num(), 2);
	TestEqual(TEXT("No battle shops in the sample chapter"), Controller->GetShopCells().Num(), 1);
	TestEqual(TEXT("Turn 1"), Controller->GetTurn(), 1);
	TestTrue(TEXT("Player phase"), Controller->IsPlayerPhase());
	TestEqual(TEXT("Gold"), Controller->GetGold(), 1000);
	TestTrue(TEXT("Ongoing"), Controller->GetResult() == ECommanderBattleResult::Ongoing);

	// Queries.
	TestFalse(TEXT("Player unit can move"), Controller->GetMovableCells(2).IsEmpty());
	TestTrue(TEXT("Enemy units cannot be moved by the player"), Controller->GetMovableCells(101).IsEmpty());
	TestTrue(TEXT("Nothing to attack yet"), Controller->GetAttackableTargets(1).IsEmpty());
	TestTrue(TEXT("Unknown unit"), Controller->GetMovableCells(77).IsEmpty());
	TestTrue(TEXT("Negative id"), Controller->GetMovableCells(-1).IsEmpty());

	// Commands: refusals first, then a real move and the gold chest at (2, 2).
	TestFalse(TEXT("Cannot enter a wall"), Controller->MoveUnit(2, 2, 1));
	TestFalse(TEXT("Cannot move the enemy"), Controller->MoveUnit(101, 3, 0));
	TestFalse(TEXT("Cannot attack from afar"), Controller->AttackUnit(1, 101));
	TestFalse(TEXT("No chest under the unit"), Controller->OpenChest(2));
	TestTrue(TEXT("Move unit 2 onto the gold chest"), Controller->MoveUnit(2, 2, 2));
	TestTrue(TEXT("Open it"), Controller->OpenChest(2));
	TestEqual(TEXT("Gold from the chest"), Controller->GetGold(), 1300);
	TestEqual(TEXT("One chest left"), Controller->GetChests().Num(), 1);
	TestFalse(TEXT("The unit has acted"), Controller->WaitUnit(2));
	TestTrue(TEXT("Wait with unit 1"), Controller->WaitUnit(1));

	// The enemy phase is played automatically.
	TestTrue(TEXT("End the player phase"), Controller->EndPlayerPhase());
	if (Controller->IsBattleRunning())
	{
		TestTrue(TEXT("Back to the player phase"), Controller->IsPlayerPhase());
		TestEqual(TEXT("Turn 2"), Controller->GetTurn(), 2);
	}

	// Pass turns until the battle ends (the sample has a turn limit of 20), then check the hand-over.
	for (int32 Guard = 0; Guard < 30 && Controller->IsBattleRunning(); ++Guard)
	{
		Controller->EndPlayerPhase();
	}
	TestFalse(TEXT("The battle ended"), Controller->IsBattleRunning());
	const ECommanderBattleResult Result = Controller->GetResult();
	const ECommanderCampaignStep Step = Controller->GetCampaignStep();
	TestTrue(TEXT("A result was reached"), Result != ECommanderBattleResult::Ongoing);
	TestTrue(TEXT("The campaign was told"), Step != ECommanderCampaignStep::BattleOngoing);
	TestFalse(TEXT("No commands after the end"), Controller->WaitUnit(1));
	TestFalse(TEXT("No end phase after the end"), Controller->EndPlayerPhase());

	if (Result == ECommanderBattleResult::Won)
	{
		TestTrue(TEXT("Won the only chapter: campaign cleared"), Step == ECommanderCampaignStep::CampaignCleared && Campaign->IsCampaignFinished());
		TestFalse(TEXT("Nothing left to fight"), Controller->BeginBattle(Campaign));
	}
	else
	{
		TestTrue(TEXT("Lost: retry the same chapter"), Step == ECommanderCampaignStep::Retry && Campaign->GetChapterIndex() == 0);
		TestTrue(TEXT("The retry can begin again"), Controller->BeginBattle(Campaign));
		TestTrue(TEXT("Fresh battle"), Controller->IsBattleRunning() && Controller->GetTurn() == 1 && Controller->GetResult() == ECommanderBattleResult::Ongoing);
	}
	return true;
}
