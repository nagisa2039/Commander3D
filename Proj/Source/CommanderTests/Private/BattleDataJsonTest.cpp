// Infrastructure tests: weapon / battle JSON -> BattleSetup -> Battle. Rule tests live in Tests/Core.

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include "Battle/BattleSetup.h"
#include "Infrastructure/BattleDataLoader.h"
#include "Infrastructure/BattleJsonParser.h"
#include "Infrastructure/ClassJsonParser.h"
#include "Infrastructure/ItemJsonParser.h"
#include "Infrastructure/WeaponJsonParser.h"

namespace
{
using commander::domain::Faction;
using commander::domain::ItemKind;
using commander::domain::WeaponSpec;

const TCHAR* ValidClasses = TEXT(R"({"classes":[
	{"id":"soldier","movement":5,"growth":{"hp":60,"strength":40,"skill":30,"speed":30,"defense":20},
	 "caps":{"hp":40,"strength":20,"speed":22},
	 "promotions":[{"level":10,"to":"knight","bonus":{"hp":3,"strength":2,"movement":1}},
	               {"level":8,"to":"scout","bonus":{"speed":3}}]},
	{"id":"knight","movement":6,"growth":{"hp":80,"strength":50},"caps":{"hp":60,"strength":30}},
	{"id":"scout","movement":7,"growth":{"speed":70},"caps":{"speed":40}},
	{"id":"lost","movement":3,"promotions":[{"level":5,"to":"nowhere"}]}
]})");

const TCHAR* ValidItems = TEXT(R"({"items":[
	{"id":"potion","kind":"potion","uses":3,"amount":10,"price":300},
	{"id":"repair","kind":"repair","uses":1},
	{"id":"heal_staff","kind":"staff","uses":5,"amount":12,"range":3,"price":2000},
	{"id":"seal","kind":"seal","uses":1,"level":5,"price":5000}
]})");

const TCHAR* ValidWeapons = TEXT(R"({"types":["sword","bow"],"weapons":[
	{"id":"sword","might":5,"hit":90,"minRange":1,"maxRange":1,"type":"sword","price":500,"avoid":-5,"rankHit":4,"rankAvoid":2},
	{"id":"bow","might":4,"hit":80,"minRange":2,"maxRange":2,"durability":3,"type":"bow","rank":1}
]})");

const TCHAR* ValidBattle = TEXT(R"({
	"map": "m",
	"conditions": {"defeatBoss": true, "turnLimit": 12},
	"units": [
		{"id":1,"side":"player","x":0,"y":1,"movement":5,"hp":18,"strength":5,"skill":4,"speed":6,"defense":2,"weapon":"sword"},
		{"id":9,"side":"enemy","x":3,"y":2,"movement":3,"hp":20,"strength":6,"skill":3,"speed":3,"defense":4,"boss":true}
	]
})");

FBattleDataTables Tables()
{
	FBattleDataTables Result;
	const auto Weapons = FWeaponJsonParser::Parse(FString(ValidWeapons));
	const auto Items = FItemJsonParser::Parse(FString(ValidItems));
	const auto Classes = FClassJsonParser::Parse(FString(ValidClasses));
	if (Weapons.IsOk() && Items.IsOk() && Classes.IsOk())
	{
		Result.Weapons = Weapons.Value(); // a catalog: weapons, type ids and prices
		Result.Items = Items.Value();
		Result.Classes = Classes.Value();
	}
	return Result;
}

EItemJsonError ItemErrorOf(const TCHAR* Json)
{
	const auto Parsed = FItemJsonParser::Parse(FString(Json));
	return Parsed.IsOk() ? EItemJsonError::None : Parsed.Error();
}

EClassJsonError ClassErrorOf(const TCHAR* Json)
{
	const auto Parsed = FClassJsonParser::Parse(FString(Json));
	return Parsed.IsOk() ? EClassJsonError::None : Parsed.Error();
}

EWeaponJsonError WeaponErrorOf(const TCHAR* Json)
{
	const auto Parsed = FWeaponJsonParser::Parse(FString(Json));
	return Parsed.IsOk() ? EWeaponJsonError::None : Parsed.Error();
}

EBattleJsonError BattleErrorOf(const TCHAR* Json)
{
	const auto Parsed = FBattleJsonParser::Parse(FString(Json), Tables());
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
	const FWeaponTable& Weapons = Parsed.Value().Weapons;
	TestEqual(TEXT("Weapon count"), Weapons.Num(), 2);
	TestEqual(TEXT("Type ids follow the list order from 1"), Parsed.Value().Types.FindRef(TEXT("bow")), 2);
	TestEqual(TEXT("Price"), Parsed.Value().Prices.FindRef(TEXT("sword")), 500);
	TestEqual(TEXT("Missing price is 0"), Parsed.Value().Prices.FindRef(TEXT("bow")), 0);
	const WeaponSpec* Bow = Weapons.Find(TEXT("bow"));
	if (!TestNotNull(TEXT("bow exists"), Bow))
	{
		return false;
	}
	TestEqual(TEXT("Might"), Bow->Might, 4);
	TestEqual(TEXT("Hit"), Bow->Hit, 80);
	TestEqual(TEXT("MinRange"), Bow->MinRange, 2);
	TestEqual(TEXT("MaxRange"), Bow->MaxRange, 2);
	TestEqual(TEXT("Durability"), Bow->Durability, 3);
	TestEqual(TEXT("Weapon type"), Bow->Type, 2);
	TestEqual(TEXT("Required rank"), Bow->RequiredRank, 1);
	const WeaponSpec* Sword = Weapons.Find(TEXT("sword"));
	TestTrue(TEXT("Durability is optional and means unbreakable"), Sword != nullptr && Sword->Durability == 0);
	TestTrue(TEXT("Rank defaults to 0"), Sword != nullptr && Sword->Type == 1 && Sword->RequiredRank == 0);
	TestTrue(TEXT("Avoid / rank bonuses read (avoid may be negative)"),
		Sword != nullptr && Sword->Avoid == -5 && Sword->RankHit == 4 && Sword->RankAvoid == 2);
	TestTrue(TEXT("Bonuses default to 0"), Bow->Avoid == 0 && Bow->RankHit == 0 && Bow->RankAvoid == 0);
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
	TestTrue(TEXT("Durability must be a whole number"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"durability":"x"}]})"))
		== EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Fractional durability"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"durability":2.5}]})"))
		== EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Unknown weapon type"),
		WeaponErrorOf(TEXT(R"({"types":["sword"],"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"type":"axe"}]})"))
		== EWeaponJsonError::InvalidField);
	TestTrue(TEXT("type needs a types list"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"type":"sword"}]})"))
		== EWeaponJsonError::InvalidField);
	TestTrue(TEXT("types must be strings"), WeaponErrorOf(TEXT(R"({"types":[1],"weapons":[]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Duplicate type name"), WeaponErrorOf(TEXT(R"({"types":["a","a"],"weapons":[]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Too many types"),
		WeaponErrorOf(TEXT(R"({"types":["a","b","c","d","e","f","g","h","i"],"weapons":[]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("Fractional rank"),
		WeaponErrorOf(TEXT(R"({"types":["a"],"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"type":"a","rank":0.5}]})"))
		== EWeaponJsonError::InvalidField);
	TestTrue(TEXT("avoid must be whole"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"avoid":2.5}]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("rankHit must be a number"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"rankHit":"x"}]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("rankAvoid must be a number"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"rankAvoid":true}]})")) == EWeaponJsonError::InvalidField);
	TestTrue(TEXT("price must be whole"),
		WeaponErrorOf(TEXT(R"({"weapons":[{"id":"a","might":1,"hit":1,"minRange":1,"maxRange":1,"price":"x"}]})")) == EWeaponJsonError::InvalidField);
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
	const auto Parsed = FBattleJsonParser::Parse(FString(ValidBattle), Tables());
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

	// A breakable weapon starts a unit with all its uses left.
	const FString BowUnit = OneUnit(FString::Printf(TEXT(R"("id":1,"side":"player","x":0,"y":0,%s,"weapon":"bow")"), Stats));
	const auto WithBow = FBattleJsonParser::Parse(BowUnit, Tables());
	if (TestTrue(TEXT("Unit with a bow parses"), WithBow.IsOk()))
	{
		const auto& Archer = WithBow.Value().Setup.Units[0];
		TestEqual(TEXT("Bow durability"), Archer.Weapon.Durability, 3);
		TestEqual(TEXT("WeaponUses starts full"), Archer.WeaponUses, 3);
	}
	TestEqual(TEXT("Level defaults to 1"), Player.Level, 1);
	TestEqual(TEXT("Experience defaults to 0"), Player.Experience, 0);
	TestTrue(TEXT("No items by default"), Player.Items.empty());

	// Class, level, experience and inventory.
	const FString Rich = OneUnit(TEXT(R"("id":3,"side":"player","x":0,"y":0,"class":"soldier","hp":18,"strength":5,"skill":4,)"
									  R"("speed":6,"defense":2,"level":4,"exp":35,"weapon":"sword","items":["bow","potion","repair"])"));
	const auto WithClass = FBattleJsonParser::Parse(Rich, Tables());
	if (TestTrue(TEXT("Unit with class and items parses"), WithClass.IsOk()))
	{
		const auto& Hero = WithClass.Value().Setup.Units[0];
		TestEqual(TEXT("Movement from the class"), Hero.Movement, 5);
		TestEqual(TEXT("Growth hp from the class"), Hero.Growth.Hp, 60);
		TestEqual(TEXT("Growth defense from the class"), Hero.Growth.Defense, 20);
		TestEqual(TEXT("Level"), Hero.Level, 4);
		TestEqual(TEXT("Experience"), Hero.Experience, 35);
		if (TestEqual(TEXT("Item count"), static_cast<int32>(Hero.Items.size()), 3))
		{
			TestTrue(TEXT("Spare weapon slot"), Hero.Items[0].Kind == ItemKind::Weapon && Hero.Items[0].Weapon.MaxRange == 2);
			TestEqual(TEXT("Spare weapon starts fresh"), Hero.Items[0].Uses, 3);
			TestTrue(TEXT("Potion slot"), Hero.Items[1].Kind == ItemKind::Potion && Hero.Items[1].Uses == 3 && Hero.Items[1].Amount == 10);
			TestTrue(TEXT("Repair slot"), Hero.Items[2].Kind == ItemKind::RepairKit && Hero.Items[2].Uses == 1);
		}
	}

	// Caps and promotion come from the class; the promotion borrows the target class's growth and caps.
	if (WithClass.IsOk())
	{
		const auto& Hero = WithClass.Value().Setup.Units[0];
		TestEqual(TEXT("Cap from the class"), Hero.Caps.Strength, 20);
		if (TestEqual(TEXT("Two promotion options resolved"), static_cast<int32>(Hero.Promotions.size()), 2))
		{
			const auto& Knight = Hero.Promotions[0];
			TestTrue(TEXT("Option 0 is the knight"), Knight.Name == "knight");
			TestEqual(TEXT("Promotion level"), Knight.MinLevel, 10);
			TestEqual(TEXT("Promotion bonus"), Knight.Bonus.Movement, 1);
			TestEqual(TEXT("New growth from the target class"), Knight.Growth.Hp, 80);
			TestEqual(TEXT("New caps from the target class"), Knight.Caps.Hp, 60);
			const auto& Scout = Hero.Promotions[1];
			TestTrue(TEXT("Option 1 is the scout"), Scout.Name == "scout");
			TestEqual(TEXT("Second option growth"), Scout.Growth.Speed, 70);
			TestEqual(TEXT("Second option caps"), Scout.Caps.Speed, 40);
		}
		TestTrue(TEXT("Item prices come from the tables"), Hero.Items[0].Price == 0 /* bow has no price */ && Hero.Items[1].Price == 300);
	}

	// Weapon ranks, in ranks (not points), by type name.
	const FString Ranked = OneUnit(FString::Printf(TEXT(R"("id":5,"side":"player","x":0,"y":0,%s,"weaponRanks":{"sword":2,"bow":1})"), Stats));
	const auto WithRanks = FBattleJsonParser::Parse(Ranked, Tables());
	if (TestTrue(TEXT("Unit with weaponRanks parses"), WithRanks.IsOk()))
	{
		const auto& Trained = WithRanks.Value().Setup.Units[0];
		TestEqual(TEXT("sword rank 2 = its threshold"), Trained.WeaponPoints[1], commander::domain::RankThreshold[2]);
		TestEqual(TEXT("bow rank 1 = its threshold"), Trained.WeaponPoints[2], commander::domain::RankThreshold[1]);
	}

	// Gold and shops (stock entries are weapon ids or item ids; prices come from the tables).
	const auto WithShop = FBattleJsonParser::Parse(
		FString(TEXT(R"({"map":"m","gold":750,"units":[],"shops":[{"x":2,"y":1,"stock":["sword","potion","heal_staff"]}]})")), Tables());
	if (TestTrue(TEXT("Gold and shops parse"), WithShop.IsOk()))
	{
		const auto& Setup = WithShop.Value().Setup;
		TestEqual(TEXT("Gold"), Setup.Gold, 750);
		if (TestEqual(TEXT("Shop count"), static_cast<int32>(Setup.Shops.size()), 1))
		{
			const auto& Shop = Setup.Shops[0];
			TestTrue(TEXT("Shop position"), Shop.Position.X == 2 && Shop.Position.Y == 1);
			TestEqual(TEXT("Stock size"), static_cast<int32>(Shop.Stock.size()), 3);
			TestTrue(TEXT("A weapon in stock is priced from the catalog"), Shop.Stock[0].Kind == ItemKind::Weapon && Shop.Stock[0].Price == 500);
			TestTrue(TEXT("Potion price"), Shop.Stock[1].Price == 300);
			TestTrue(TEXT("Staff in stock"), Shop.Stock[2].Kind == ItemKind::Staff && Shop.Stock[2].Price == 2000);
		}
	}

	// An explicit movement overrides the class.
	const FString Override = OneUnit(TEXT(R"("id":4,"side":"player","x":0,"y":0,"class":"soldier","movement":2,"hp":9,)"
										  R"("strength":1,"skill":1,"speed":1,"defense":1)"));
	const auto WithOverride = FBattleJsonParser::Parse(Override, Tables());
	TestTrue(TEXT("Explicit movement wins"), WithOverride.IsOk() && WithOverride.Value().Setup.Units[0].Movement == 2);
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
	TestTrue(TEXT("map must be a string, not a number"), BattleErrorOf(TEXT(R"({"map":1,"units":[]})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("side must be a string"),
		BattleErrorOf(*OneUnit(FString::Printf(TEXT(R"("id":1,"side":true,"x":0,"y":0,%s)"), Stats))) == EBattleJsonError::InvalidField);
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
	const FString NoMove = TEXT(R"("id":1,"side":"player","x":0,"y":0,"hp":9,"strength":1,"skill":1,"speed":1,"defense":1)");
	TestTrue(TEXT("movement is required without a class"), BattleErrorOf(*OneUnit(NoMove)) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("A class supplies the movement"),
		BattleErrorOf(*OneUnit(NoMove + TEXT(R"(,"class":"scout")"))) == EBattleJsonError::None);
	TestTrue(TEXT("Unknown class"), BattleErrorOf(*OneUnit(NoMove + TEXT(R"(,"class":"wizard")"))) == EBattleJsonError::UnknownClass);
	TestTrue(TEXT("class must be a string"), BattleErrorOf(*OneUnit(NoMove + TEXT(R"(,"class":1)"))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Unknown item"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"items":["elixir"])"))) == EBattleJsonError::UnknownItem);
	TestTrue(TEXT("items must be strings"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"items":[1])"))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("items must be an array"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"items":"potion")"))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Known items"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"items":["potion","bow"])"))) == EBattleJsonError::None);
	TestTrue(TEXT("level must be whole"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"level":1.5)"))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("exp must be a number"), BattleErrorOf(*OneUnit(Base + TEXT(R"(,"exp":"x")"))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Unknown promotion target"), BattleErrorOf(*OneUnit(NoMove + TEXT(R"(,"class":"lost")"))) == EBattleJsonError::UnknownClass);
	TestTrue(TEXT("Unknown weapon type in weaponRanks"),
		BattleErrorOf(*OneUnit(Base + TEXT(R"(,"weaponRanks":{"axe":1})"))) == EBattleJsonError::UnknownWeaponType);
	TestTrue(TEXT("weaponRanks values must be whole"),
		BattleErrorOf(*OneUnit(Base + TEXT(R"(,"weaponRanks":{"sword":1.5})"))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("weaponRanks must be an object"),
		BattleErrorOf(*OneUnit(Base + TEXT(R"(,"weaponRanks":[1])"))) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("gold must be a number"), BattleErrorOf(TEXT(R"({"map":"m","gold":"rich","units":[]})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("shops must be an array"), BattleErrorOf(TEXT(R"({"map":"m","units":[],"shops":{}})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("shop needs a position"),
		BattleErrorOf(TEXT(R"({"map":"m","units":[],"shops":[{"x":1,"stock":["potion"]}]})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("shop needs a stock"),
		BattleErrorOf(TEXT(R"({"map":"m","units":[],"shops":[{"x":1,"y":1}]})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("Unknown stock id"),
		BattleErrorOf(TEXT(R"({"map":"m","units":[],"shops":[{"x":1,"y":1,"stock":["elixir"]}]})")) == EBattleJsonError::UnknownItem);
	TestTrue(TEXT("Negative turnLimit"),
		BattleErrorOf(TEXT(R"({"map":"m","conditions":{"turnLimit":-1},"units":[]})")) == EBattleJsonError::InvalidField);
	TestTrue(TEXT("conditions must be an object"),
		BattleErrorOf(TEXT(R"({"map":"m","conditions":3,"units":[]})")) == EBattleJsonError::InvalidField);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderClassItemJsonTest,
	"Commander3D.Json.ParsesClassesAndItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderClassItemJsonTest::RunTest(const FString& Parameters)
{
	const auto Classes = FClassJsonParser::Parse(FString(ValidClasses));
	if (TestTrue(TEXT("Classes parse"), Classes.IsOk()))
	{
		const FClassSpec* Soldier = Classes.Value().Find(TEXT("soldier"));
		TestTrue(TEXT("soldier read"), Soldier != nullptr && Soldier->Movement == 5 && Soldier->Growth.Speed == 30);
		const FClassSpec* Scout = Classes.Value().Find(TEXT("scout"));
		TestTrue(TEXT("growth is optional and defaults to 0"), Scout != nullptr && Scout->Movement == 7 && Scout->Growth.Hp == 0);
		TestTrue(TEXT("caps read, missing keys are 0 (no cap)"),
			Soldier != nullptr && Soldier->Caps.Hp == 40 && Soldier->Caps.Strength == 20 && Soldier->Caps.Speed == 22 && Soldier->Caps.Skill == 0);
		if (TestTrue(TEXT("two promotion options read"), Soldier != nullptr && Soldier->Promotions.Num() == 2))
		{
			const FClassPromotion& First = Soldier->Promotions[0];
			TestTrue(TEXT("first option"),
				First.MinLevel == 10 && First.TargetClass == TEXT("knight") && First.Bonus.Hp == 3 && First.Bonus.Strength == 2
					&& First.Bonus.Movement == 1 && First.Bonus.Skill == 0);
			const FClassPromotion& Second = Soldier->Promotions[1];
			TestTrue(TEXT("second option"), Second.MinLevel == 8 && Second.TargetClass == TEXT("scout") && Second.Bonus.Speed == 3);
		}
		TestTrue(TEXT("no promotions by default"), Scout != nullptr && Scout->Promotions.IsEmpty());
	}
	TestTrue(TEXT("Class: promotion option needs a level"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"promotions":[{"to":"b"}]}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: promotion option needs a target"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"promotions":[{"level":5}]}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: promotions must be an array"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"promotions":{"level":5,"to":"b"}}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: promotion options must be objects"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"promotions":["b"]}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: duplicate promotion target"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"promotions":[{"level":5,"to":"b"},{"level":6,"to":"b"}]}]})"))
		== EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: too many promotion options"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"promotions":[
			{"level":5,"to":"b"},{"level":5,"to":"c"},{"level":5,"to":"d"},{"level":5,"to":"e"},{"level":5,"to":"f"}]}]})"))
		== EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: caps must be an object"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"caps":1}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: fractional cap"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"caps":{"hp":1.5}}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: fractional bonus"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"promotions":[{"level":5,"to":"b","bonus":{"hp":0.5}}]}]})"))
		== EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: broken JSON"), ClassErrorOf(TEXT("[")) == EClassJsonError::InvalidJson);
	TestTrue(TEXT("Class: missing array"), ClassErrorOf(TEXT("{}")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: missing movement"), ClassErrorOf(TEXT(R"({"classes":[{"id":"a"}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: growth must be an object"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"growth":5}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: fractional growth"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3,"growth":{"hp":2.5}}]})")) == EClassJsonError::InvalidField);
	TestTrue(TEXT("Class: duplicate id"),
		ClassErrorOf(TEXT(R"({"classes":[{"id":"a","movement":3},{"id":"a","movement":4}]})")) == EClassJsonError::InvalidField);

	const auto Items = FItemJsonParser::Parse(FString(ValidItems));
	if (TestTrue(TEXT("Items parse"), Items.IsOk()))
	{
		const auto* Potion = Items.Value().Find(TEXT("potion"));
		TestTrue(TEXT("potion read"), Potion != nullptr && Potion->Kind == ItemKind::Potion && Potion->Uses == 3 && Potion->Amount == 10);
		TestTrue(TEXT("potion price"), Potion != nullptr && Potion->Price == 300);
		const auto* Staff = Items.Value().Find(TEXT("heal_staff"));
		const auto* Seal = Items.Value().Find(TEXT("seal"));
		TestTrue(TEXT("seal read (the level becomes Amount)"),
			Seal != nullptr && Seal->Kind == ItemKind::PromotionSeal && Seal->Uses == 1 && Seal->Amount == 5 && Seal->Price == 5000);
		TestTrue(TEXT("staff read"),
			Staff != nullptr && Staff->Kind == ItemKind::Staff && Staff->Uses == 5 && Staff->Amount == 12 && Staff->Range == 3 && Staff->Price == 2000);
		const auto* Repair = Items.Value().Find(TEXT("repair"));
		TestTrue(TEXT("repair read"), Repair != nullptr && Repair->Kind == ItemKind::RepairKit && Repair->Uses == 1);
	}
	TestTrue(TEXT("Item: broken JSON"), ItemErrorOf(TEXT("nope")) == EItemJsonError::InvalidJson);
	TestTrue(TEXT("Item: missing array"), ItemErrorOf(TEXT("{}")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: unknown kind"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"scroll","uses":1}]})")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: potion needs amount"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"potion","uses":1}]})")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: seal needs a level"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"seal","uses":1}]})")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: seal level must be whole"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"seal","uses":1,"level":2.5}]})")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: staff needs range"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"staff","uses":1,"amount":5}]})")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: staff needs amount"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"staff","uses":1,"range":2}]})")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: price must be whole"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"repair","uses":1,"price":1.5}]})")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: missing uses"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"repair"}]})")) == EItemJsonError::InvalidField);
	TestTrue(TEXT("Item: duplicate id"),
		ItemErrorOf(TEXT(R"({"items":[{"id":"a","kind":"repair","uses":1},{"id":"a","kind":"repair","uses":2}]})"))
		== EItemJsonError::InvalidField);
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
		TestEqual(TEXT("Unit count"), static_cast<int32>(Started.Value().Units().size()), 5);
		TestEqual(TEXT("Map width from test_map"), Started.Value().GetMap().GetGrid().Width(), 5);
		const auto* Hero = Started.Value().FindUnit(commander::domain::UnitId{1});
		if (TestNotNull(TEXT("Unit 1 exists"), Hero))
		{
			TestTrue(TEXT("Sample hero uses class growth"), Hero->Growth.Hp > 0 && Hero->Movement > 0);
			TestTrue(TEXT("Sample hero carries items"), !Hero->Items.empty());
			TestTrue(TEXT("Sample hero has promotion options"), Hero->Promotions.size() == 2 && Hero->Caps.Strength > 0);
			TestTrue(TEXT("Sample hero starts with sword rank 1"), commander::domain::WeaponRankOf(Hero->WeaponPoints[1]) == 1);
		}
		TestEqual(TEXT("Sample gold"), Started.Value().Gold(), 1000);
		TestEqual(TEXT("Sample shop count"), static_cast<int32>(Started.Value().Shops().size()), 1);
	}

	TestTrue(TEXT("Missing battle file"),
		FBattleDataLoader::Load(DataDir, TEXT("no_such_battle")).Error() == EBattleLoadError::FileNotReadable);
	TestTrue(TEXT("Path traversal rejected"),
		FBattleDataLoader::Load(DataDir, TEXT("../Maps/test_map")).Error() == EBattleLoadError::InvalidName);
	TestTrue(TEXT("Empty name rejected"), FBattleDataLoader::Load(DataDir, TEXT("")).Error() == EBattleLoadError::InvalidName);
	return true;
}
