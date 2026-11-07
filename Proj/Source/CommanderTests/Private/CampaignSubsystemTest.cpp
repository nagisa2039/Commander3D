// Presentation test: the Blueprint-facing subsystem forwards to Campaign / Preparation and translates the results.

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "Engine/GameInstance.h"

#include "Presentation/CommanderCampaignSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderCampaignSubsystemTest,
	"Commander3D.Presentation.CampaignSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderCampaignSubsystemTest::RunTest(const FString& Parameters)
{
	// A game-instance subsystem must live inside a game instance (not initialised: only the object is needed).
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UCommanderCampaignSubsystem* Subsystem = NewObject<UCommanderCampaignSubsystem>(GameInstance);
	if (!TestNotNull(TEXT("Subsystem is created"), Subsystem))
	{
		return false;
	}

	// Nothing started yet: every call is a harmless no-op.
	TestFalse(TEXT("No campaign yet"), Subsystem->HasCampaign());
	TestTrue(TEXT("No roster"), Subsystem->GetRoster().IsEmpty());
	TestFalse(TEXT("Deploy needs a campaign"), Subsystem->Deploy(1));
	TestFalse(TEXT("Save needs a campaign"), Subsystem->SaveGame(TEXT("nope")));
	TestFalse(TEXT("Unknown campaign"), Subsystem->StartCampaign(TEXT("no_such_campaign"), true));
	TestFalse(TEXT("Unsafe campaign name"), Subsystem->StartCampaign(TEXT("../x"), true));

	if (!TestTrue(TEXT("Sample campaign starts"), Subsystem->StartCampaign(TEXT("test_campaign"), true)))
	{
		return false;
	}
	TestEqual(TEXT("Chapter name"), Subsystem->GetChapterName(), FString(TEXT("Test Chapter")));
	TestEqual(TEXT("Chapter index"), Subsystem->GetChapterIndex(), 0);
	TestEqual(TEXT("Gold"), Subsystem->GetGold(), 1000);
	TestEqual(TEXT("Deploy limit"), Subsystem->GetMaxDeployed(), 2);

	TArray<FCommanderUnitView> Roster = Subsystem->GetRoster();
	if (!TestEqual(TEXT("Two members"), Roster.Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("Member id"), Roster[0].Id, 1);
	TestTrue(TEXT("Has an equipped sword"), Roster[0].bHasWeapon && Roster[0].Weapon.MaxRange == 1 && Roster[0].Weapon.Might > 0);
	TestTrue(TEXT("Carries a potion"), Roster[0].Items.Num() == 2 && Roster[0].Items[0].Kind == ECommanderItemKind::Potion);
	TestFalse(TEXT("Nobody deployed yet"), Roster[0].bDeployed);

	// Deploy / withdraw, with bad input.
	TestTrue(TEXT("Deploy 1"), Subsystem->Deploy(1));
	TestFalse(TEXT("Deploy again"), Subsystem->Deploy(1));
	TestFalse(TEXT("Negative id"), Subsystem->Deploy(-1));
	TestEqual(TEXT("Deployed count"), Subsystem->GetDeployedCount(), 1);
	TestTrue(TEXT("Roster shows the deployment"), Subsystem->GetRoster()[0].bDeployed);
	TestTrue(TEXT("Withdraw 1"), Subsystem->Withdraw(1));
	TestFalse(TEXT("Withdraw again"), Subsystem->Withdraw(1));

	// Shopping and item organizing.
	const TArray<FCommanderItemView> Stock = Subsystem->GetStock();
	if (TestEqual(TEXT("Stock size"), Stock.Num(), 3))
	{
		TestTrue(TEXT("First stock entry is priced"), Stock[0].Price > 0);
		const int32 GoldBefore = Subsystem->GetGold();
		TestTrue(TEXT("Buy"), Subsystem->BuyItem(2, 0));
		TestEqual(TEXT("Gold paid"), Subsystem->GetGold(), GoldBefore - Stock[0].Price);
		TestFalse(TEXT("Negative entry"), Subsystem->BuyItem(2, -1));
		TestFalse(TEXT("Entry out of range"), Subsystem->BuyItem(2, 99));
		TestTrue(TEXT("Sell it back"), Subsystem->SellItem(2, 2)); // unit 2 had two items; the new one is in slot 2
		TestEqual(TEXT("Gold refunded by half"), Subsystem->GetGold(), GoldBefore - Stock[0].Price + Stock[0].Price / 2);
	}
	TestFalse(TEXT("Negative slot"), Subsystem->DiscardItem(1, -1));
	TestTrue(TEXT("Transfer"), Subsystem->TransferItem(1, 2, 0));
	TestTrue(TEXT("Discard"), Subsystem->DiscardItem(1, 0));

	// Save and load through the slot directory.
	const FString Slot = TEXT("AutomationSubsystemSlot");
	TestTrue(TEXT("Save"), Subsystem->SaveGame(Slot));
	UCommanderCampaignSubsystem* Other = NewObject<UCommanderCampaignSubsystem>(GameInstance);
	TestTrue(TEXT("Load"), Other->LoadGame(Slot));
	TestEqual(TEXT("Loaded gold"), Other->GetGold(), Subsystem->GetGold());
	TestEqual(TEXT("Loaded roster"), Other->GetRoster().Num(), 2);
	TestTrue(TEXT("Loaded campaign can be saved again"), Other->SaveGame(Slot));
	TestFalse(TEXT("Missing slot"), Other->LoadGame(TEXT("no_such_slot")));
	IFileManager::Get().Delete(*FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), Slot + TEXT(".json")));

	// Battle hand-over: nobody deployed -> no battle; deployed -> a battle.
	TestFalse(TEXT("No battle without a sortie"), Subsystem->StartBattle().has_value());
	Subsystem->Deploy(1);
	const auto Prepared = Subsystem->StartBattle();
	TestTrue(TEXT("Battle starts"), Prepared.has_value());
	if (Prepared.has_value())
	{
		TestEqual(TEXT("Ongoing battle is not reported as progress"), static_cast<int32>(Subsystem->CompleteBattle(Prepared->Battle, Prepared->Deployed)),
			static_cast<int32>(ECommanderCampaignStep::BattleOngoing));
		TestEqual(TEXT("Still chapter 0"), Subsystem->GetChapterIndex(), 0);
	}
	return true;
}
