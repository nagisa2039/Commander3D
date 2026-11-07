// Infrastructure test: campaign JSON -> Roster + chapters -> Campaign -> Preparation -> Battle.

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include "Battle/Campaign.h"
#include "Infrastructure/BattleJsonParser.h"
#include "Infrastructure/CampaignLoader.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderCampaignLoaderTest,
	"Commander3D.Json.LoadsSampleCampaign",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderCampaignLoaderTest::RunTest(const FString& Parameters)
{
	using namespace commander;
	const FString DataDir = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));

	const auto Loaded = FCampaignLoader::Load(DataDir, TEXT("test_campaign"));
	if (!TestTrue(TEXT("Sample campaign loads"), Loaded.IsOk()))
	{
		return false;
	}
	const FCampaignDefinition& Definition = Loaded.Value();
	TestEqual(TEXT("Roster size"), static_cast<int32>(Definition.InitialRoster.Members().size()), 2);
	TestEqual(TEXT("Roster gold"), Definition.InitialRoster.Gold(), 1000);
	TestEqual(TEXT("Chapter count"), static_cast<int32>(Definition.Chapters.size()), 1);
	if (Definition.Chapters.empty() || Definition.InitialRoster.Members().size() != 2)
	{
		return false;
	}
	TestEqual(TEXT("Chapter name"), FString(UTF8_TO_TCHAR(Definition.Chapters[0].Name.c_str())), FString(TEXT("Test Chapter")));
	TestEqual(TEXT("Start cell count"), static_cast<int32>(Definition.Chapters[0].StartCells.size()), 2);

	const domain::BattleUnit* Hero = Definition.InitialRoster.Find(domain::UnitId{1});
	if (TestNotNull(TEXT("Roster unit 1"), Hero))
	{
		TestTrue(TEXT("Roster unit is a player"), Hero->Side == domain::Faction::Player);
		TestTrue(TEXT("Roster unit uses its class"), Hero->Movement > 0 && Hero->Growth.Hp > 0);
	}

	// The whole flow: campaign -> preparation -> battle.
	application::Campaign Run(Definition.InitialRoster, domain::Difficulty::Classic, Definition.Chapters);
	TestTrue(TEXT("Deploy 1"), Run.Preparation().Deploy(domain::UnitId{1}));
	TestTrue(TEXT("Deploy 2"), Run.Preparation().Deploy(domain::UnitId{2}));
	const auto Started = Run.Preparation().Start();
	TestTrue(TEXT("Chapter battle starts"), Started.IsOk());
	if (Started.IsOk())
	{
		TestEqual(TEXT("Units on the map"), static_cast<int32>(Started.Value().Battle.Units().size()), 4);
		TestEqual(TEXT("Gold carried in"), Started.Value().Battle.Gold(), 1000);
	}

	// Parser details: "name" and "startCells" are optional and strictly typed; roster entries need no position or side.
	const FBattleDataTables NoTables;
	const TCHAR* Unit = TEXT(R"({"id":9,"side":"enemy","x":1,"y":0,"movement":3,"hp":5,"strength":1,"skill":1,"speed":1,"defense":0})");
	const auto Parse = [&](const FString& Extra) { return FBattleJsonParser::Parse(FString::Printf(TEXT(R"({"map":"m","units":[%s]%s})"), Unit, *Extra), NoTables); };
	const auto Plain = Parse(TEXT(""));
	TestTrue(TEXT("No name or start cells is fine"), Plain.IsOk() && Plain.Value().Name.IsEmpty() && Plain.Value().StartCells.IsEmpty());
	const auto Cells = Parse(TEXT(R"(,"name":"A","startCells":[{"x":1,"y":2}])"));
	TestTrue(TEXT("Name and start cells read"), Cells.IsOk() && Cells.Value().Name == TEXT("A") && Cells.Value().StartCells.Num() == 1 && Cells.Value().StartCells[0].Y == 2);
	TestTrue(TEXT("startCells must be an array"), !Parse(TEXT(R"(,"startCells":1)")).IsOk());
	TestTrue(TEXT("start cell needs y"), !Parse(TEXT(R"(,"startCells":[{"x":1}])")).IsOk());
	TestTrue(TEXT("name must be a string"), !Parse(TEXT(R"(,"name":3)")).IsOk());

	TSharedPtr<FJsonObject> RosterObject;
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(FString(TEXT(R"({"id":4,"movement":3,"hp":5,"strength":1,"skill":1,"speed":1,"defense":0})"))), RosterObject);
	domain::BattleUnit RosterUnit;
	TestTrue(TEXT("Roster entry without side or position"), FBattleJsonParser::ParseRosterUnit(RosterObject, NoTables, RosterUnit) == EBattleJsonError::None);
	TestTrue(TEXT("Roster entry is a player"), RosterUnit.Side == domain::Faction::Player && RosterUnit.Id.Value == 4);

	TestTrue(TEXT("Missing campaign"), FCampaignLoader::Load(DataDir, TEXT("no_such_campaign")).Error() == ECampaignLoadError::FileNotReadable);
	TestTrue(TEXT("Path traversal rejected"), FCampaignLoader::Load(DataDir, TEXT("../Weapons")).Error() == ECampaignLoadError::InvalidName);
	return true;
}
