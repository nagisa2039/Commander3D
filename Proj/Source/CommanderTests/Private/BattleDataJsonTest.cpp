// Infrastructure tests: weapon / battle JSON -> BattleSetup -> Battle. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include "Battle/BattleSetup.h"
#include "Infrastructure/BattleDataLoader.h"
#include "Infrastructure/BattleJsonParser.h"
#include "Infrastructure/WeaponJsonParser.h"

namespace
{
using commander::domain::Faction;
using commander::domain::WeaponSpec;

const TCHAR* ValidWeapons = TEXT(R"({"weapons":[
	{"id":"sword","might":5,"hit":90,"minRange":1,"maxRange":1},
	{"id":"bow","might":4,"hit":80,"minRange":2,"maxRange":2}
]})");

const TCHAR* ValidBattle = TEXT(R"({
	"map": "m",
	"conditions": {"defeatBoss": true, "turnLimit": 12},
	"units": [
		{"id":1,"side":"player","x":0,"y":1,"movement":5,"hp":18,"strength":5,"skill":4,"speed":6,"defense":2,"weapon":"sword"},
		{"id":9,"side":"enemy","x":3,"y":2,"movement":3,"hp":20,"strength":6,"skill":3,"speed":3,"defense":4,"boss":true}
	]
})");

FWeaponTable Table()
{
	const auto Parsed = FWeaponJsonParser::Parse(FString(ValidWeapons));
	return Parsed.IsOk() ? Parsed.Value() : FWeaponTable();
}

EWeaponJsonError WeaponErrorOf(const TCHAR* Json)
{
	const auto Parsed = FWeaponJsonParser::Parse(FString(Json));
	return Parsed.IsOk() ? EWeaponJsonError::None : Parsed.Error();
}

EBattleJsonError BattleErrorOf(const TCHAR* Json)
{
	const auto Parsed = FBattleJsonParser::Parse(FString(Json), Table());
	return Parsed.IsOk() ? EBattleJsonError::None : Parsed.Error();
}

// One unit entry with a replaceable fragment, to build invalid variants.
FString OneUnit(const FString& Fields)
{
	return FString::Printf(TEXT(R"({"map":"m","units":[{%s}]})"), *Fields);
}

const TCHAR* Stats = TEXT(R"("movement":3,"hp":10,"strength":1,"skill":1,"speed":1,"defense":1)");
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderWeaponJsonValidTest,
	"Commander3D.Json.ParsesWeapons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderWeaponJsonValidTest::RunTest(const FString& Parameters)
{
	const auto Parsed = FWeaponJsonParser::Parse(FString(ValidWeapons));
	if (!TestTrue(TEXT("Valid weapons parse"), Parsed.IsOk()))
	{
		return false;
	}
	const FWeaponTable& Weapons = Parsed.Value();
	TestEqual(TEXT("Weapon count"), Weapons.Num(), 2);
	const WeaponSpec* Bow = Weapons.Find(TEXT("bow"));
	if (!TestNotNull(TEXT("bow exists"), Bow))
	{
		return false;
	}
	TestEqual(TEXT("Might"), Bow->Might, 4);
	TestEqual(TEXT("Hit"), Bow->Hit, 80);
	TestEqual(TEXT("MinRange"), Bow->MinRange, 2);
	TestEqual(TEXT("MaxRange"), Bow->MaxRange, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderWeaponJsonErrorsTest,
	"Commander3D.Json.RejectsInvalidWeapons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderWeaponJsonErrorsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Broken JSON"), WeaponErrorOf(TEXT("{ nope")) == EWeaponJsonError::InvalidJson);
	TestTrue(TEXT("Missing weapons array"), WeaponErrorOf(TEXT("{}")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Missing id"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"might":1,"hit":1,"minRange":1,"maxRange":1}]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Fractional might"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1.5,"hit":1,"minRange":1,"maxRange":1}]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Missing maxRange"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1,"hit":1,"minRange":1}]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Duplicate id"),
		WeaponErrorOf(TEXT(R"({"weapons":[
			{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1},
			{"id":"a","might":2,"hit":1,"minRange":1,"maxRange":1}]})")) == EWeaponJsonError::InvalidField);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderBattleJsonValidTest,
	"Commander3D.Json.ParsesBattle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderBattleJsonValidTest::RunTest(const FString& Parameters)
{
	const auto Parsed = FBattleJsonParser::Parse(FString(ValidBattle), Table());
	if (!TestTrue(TEXT("Valid battle parses"), Parsed.IsOk()))
	{
		return false;
	}
	const FBattleDefinition& Def = Parsed.Value();
	TestEqual(TEXT("Map name"), Def.MapName, FString(TEXT("m")));
	TestTrue(TEXT("routEnemies defaults to true"), Def.Setup.Conditions.RoutEnemies);
	TestTrue(TEXT("defeatBoss read"), Def.Setup.Conditions.DefeatBoss);
	TestEqual(TEXT("turnLimit read"), Def.Setup.Conditions.TurnLimit, 12);
	if (!TestEqual(TEXT("Unit count"), static_cast<int32>(Def.Setup.Units.size()), 2))
	{
		return false;
	}
	const auto& Player = Def.Setup.Units[0];
	TestEqual(TEXT("id"), static_cast<int32>(Player.Id.Value), 1);
	TestTrue(TEXT("side"), Player.Side == Faction::Player);
	TestEqual(TEXT("x"), Player.Position.X, 0);
	TestEqual(TEXT("y"), Player.Position.Y, 1);
	TestEqual(TEXT("hp"), Player.Hp, 18);
	TestEqual(TEXT("MaxHp follows hp"), Player.MaxHp, 18);
	TestEqual(TEXT("weapon might"), Player.Weapon.Might, 5);
	TestFalse(TEXT("boss defaults to false"), Player.IsBoss);
	const auto& Boss = Def.Setup.Units[1];
	TestTrue(TEXT("enemy side"), Boss.Side == Faction::Enemy);
	TestTrue(TEXT("boss flag"), Boss.IsBoss);
	TestEqual(TEXT("no weapon means MaxRange 0"), Boss.Weapon.MaxRange, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderBattleJsonErrorsTest,
	"Commander3D.Json.RejectsInvalidBattle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderBattleJsonErrorsTest::RunTest(const FString& Parameters)
{
	const FString Base = FString::Printf(TEXT(R"("id":1,"side":"player","x":0,"y":0,%s)"), Stats);
	TestTrue(TEXT("Broken JSON"), BattleErrorOf(TEXT("[")) == EBattleJsonError::InvalidJson);
	TestTrue(TEXT("Missing map"), BattleErrorOf(TEXT(R"({"units":[]})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Missing units"), BattleErrorOf(TEXT(R"({"map":"m"})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Minimal unit is accepted"), BattleErrorOf(*OneUnit(Base)) == EBattleJsonError::None);
	TestTrue(TEXT("Unknown side"),
		BattleErrorOf(*OneUnit(FString::Printf(TEXT(R"("id":1,"side":"ally","x":0,"y":0,%s)"), Stats))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Missing hp"),
		BattleErrorOf(*OneUnit(TEXT(R"("id":1,"side":"player","x":0,"y":0,"movement":3,"strength":1,"skill":1,"speed":1,"defense":1)")))
		== EBattleJsonError::InvalidField);
	TestTrue(TEXT("Negative id"),
		BattleErrorOf(*OneUnit(FString::Printf(TEXT(R"("id":-1,"side":"player","x":0,"y":0,%s)"), Stats))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Fractional x"),
		BattleErrorOf(*OneUnit(FString::Printf(TEXT(R"("id":1,"side":"player","x":0.5,"y":0,%s)"), Stats))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("boss must be bool"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"boss":1)"))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Unknown weapon"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"weapon":"axe")"))) == EBattleJsonError::UnknownWeapon);
	TestTrue(TEXT("Known weapon"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"weapon":"bow")"))) == EBattleJsonError::None);
	TestTrue(TEXT("Negative turnLimit"),
		BattleErrorOf(TEXT(R"({"map":"m","conditions":{"turnLimit":-1},"units":[]})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("conditions must be an object"),
		BattleErrorOf(TEXT(R"({"map":"m","conditions":3,"units":[]})")) == EBattleJsonError::InvalidField);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderBattleDataLoaderTest,
	"Commander3D.Json.LoadsSampleBattle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderBattleDataLoaderTest::RunTest(const FString& Parameters)
{
	const FString DataDir = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));

	const auto Loaded = FBattleDataLoader::Load(DataDir, TEXT("test_battle"));
	if (!TestTrue(TEXT("Sample battle loads"), Loaded.IsOk()))
	{
		return false;
	}
	const auto Started = commander::application::StartBattle(Loaded.Value());
	TestTrue(TEXT("Sample battle starts"), Started.IsOk());
	if (Started.IsOk())
	{
		TestEqual(TEXT("Unit count"), static_cast<int32>(Started.Value().Units().size()), 4);
		TestEqual(TEXT("Map width from test_map"), Started.Value().GetMap().GetGrid().Width(), 5);
	}

	TestTrue(TEXT("Missing battle file"),
		FBattleDataLoader::Load(DataDir, TEXT("no_such_battle")).Error() == EBattleLoadError::FileNotReadable);
	TestTrue(TEXT("Path traversal rejected"),
		FBattleDataLoader::Load(DataDir, TEXT("../Maps/test_map")).Error() == EBattleLoadError::InvalidName);
	TestTrue(TEXT("Empty name rejected"), FBattleDataLoader::Load(DataDir, TEXT("")).Error() == EBattleLoadError::InvalidName);
	return true;
}
