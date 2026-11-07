// Infrastructure tests: save JSON round trip and save slot storage.

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Infrastructure/BattleDataLoader.h"
#include "Infrastructure/CampaignLoader.h"
#include "Infrastructure/SaveJsonParser.h"
#include "Infrastructure/SaveStorage.h"

namespace
{
using namespace commander::domain;

FString DataDir()
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"));
}

// A unit with every persistent field set to something non-default.
BattleUnit FullUnit()
{
	BattleUnit U;
	U.Id = UnitId{7};
	U.Side = Faction::Player;
	U.Movement = 5;
	U.Hp = 18;
	U.MaxHp = 18;
	U.Strength = 6;
	U.Skill = 5;
	U.Speed = 4;
	U.Defense = 3;
	U.Level = 4;
	U.Experience = 55;
	U.IsBoss = true;
	U.Weapon = WeaponSpec{5, 90, 1, 2, 30, 1, 1, -5, 4, 2};
	U.WeaponUses = 12;
	U.Growth = GrowthRates{60, 40, 30, 20, 10};
	U.Caps = StatCaps{40, 20, 22, 25, 18};
	PromotionSpec Promotion;
	Promotion.Name = "knight";
	Promotion.MinLevel = 10;
	Promotion.Bonus = StatBonus{3, 2, 1, 0, 1, 1};
	Promotion.Growth = GrowthRates{80, 50, 0, 0, 0};
	Promotion.Caps = StatCaps{60, 30, 0, 0, 0};
	U.Promotions.push_back(Promotion);
	ItemSlot Spare;
	Spare.Kind = ItemKind::Weapon;
	Spare.Weapon = WeaponSpec{4, 80, 2, 2, 3, 2, 0, 0, 0, 0};
	Spare.Uses = 2;
	Spare.Price = 800;
	ItemSlot Staff;
	Staff.Kind = ItemKind::Staff;
	Staff.Uses = 4;
	Staff.Amount = 12;
	Staff.Range = 3;
	U.Items = {Spare, Staff};
	U.WeaponPoints[1] = 120; // "sword"
	return U;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderSaveJsonTest,
	"Commander3D.Save.JsonRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderSaveJsonTest::RunTest(const FString& Parameters)
{
	const auto Tables = FBattleDataLoader::LoadTables(DataDir());
	if (!TestTrue(TEXT("Tables load"), Tables.IsOk()))
	{
		return false;
	}
	FSaveData Data;
	Data.CampaignName = TEXT("test_campaign");
	Data.Difficulty = Difficulty::Casual;
	Data.ChapterIndex = 2;
	Data.Roster.SetGold(450);
	TestTrue(TEXT("Add unit"), Data.Roster.Add(FullUnit()));

	const auto Text = FSaveJsonParser::Serialize(Data, Tables.Value());
	if (!TestTrue(TEXT("Serializes"), Text.IsOk()))
	{
		return false;
	}
	const auto Back = FSaveJsonParser::Parse(Text.Value(), Tables.Value());
	if (!TestTrue(TEXT("Parses back"), Back.IsOk()))
	{
		return false;
	}
	const FSaveData& B = Back.Value();
	TestEqual(TEXT("Campaign"), B.CampaignName, Data.CampaignName);
	TestTrue(TEXT("Difficulty"), B.Difficulty == Difficulty::Casual);
	TestEqual(TEXT("Chapter"), B.ChapterIndex, 2);
	TestEqual(TEXT("Gold"), B.Roster.Gold(), 450);

	const BattleUnit* U = B.Roster.Find(UnitId{7});
	if (!TestNotNull(TEXT("Unit survives"), U))
	{
		return false;
	}
	TestTrue(TEXT("Scalar stats"), U->Movement == 5 && U->MaxHp == 18 && U->Hp == 18 && U->Strength == 6 && U->Skill == 5 && U->Speed == 4
		&& U->Defense == 3 && U->Level == 4 && U->Experience == 55 && U->IsBoss && U->Side == Faction::Player);
	TestTrue(TEXT("Weapon"), U->WeaponUses == 12 && U->Weapon.Might == 5 && U->Weapon.Avoid == -5 && U->Weapon.RankAvoid == 2
		&& U->Weapon.Type == 1 && U->Weapon.Durability == 30);
	TestTrue(TEXT("Growth and caps"), U->Growth.Hp == 60 && U->Growth.Defense == 10 && U->Caps.Speed == 25 && U->Caps.Defense == 18);
	TestTrue(TEXT("Promotion"), U->Promotions.size() == 1 && U->Promotions[0].Name == "knight" && U->Promotions[0].MinLevel == 10
		&& U->Promotions[0].Bonus.Movement == 1 && U->Promotions[0].Growth.Hp == 80 && U->Promotions[0].Caps.Strength == 30);
	TestTrue(TEXT("Items"), U->Items.size() == 2 && U->Items[0].Kind == ItemKind::Weapon && U->Items[0].Weapon.MinRange == 2 && U->Items[0].Uses == 2
		&& U->Items[0].Price == 800 && U->Items[1].Kind == ItemKind::Staff && U->Items[1].Range == 3 && U->Items[1].Amount == 12);
	TestEqual(TEXT("Sword training by name"), U->WeaponPoints[1], 120);
	TestEqual(TEXT("Other training"), U->WeaponPoints[2], 0);
	TestTrue(TEXT("JSON names the weapon type"), Text.Value().Contains(TEXT("\"sword\"")));

	TestTrue(TEXT("Garbage"), FSaveJsonParser::Parse(TEXT("nope"), Tables.Value()).Error() == ESaveJsonError::InvalidJson);
	TestTrue(TEXT("No version"), FSaveJsonParser::Parse(TEXT("{}"), Tables.Value()).Error() == ESaveJsonError::UnsupportedVersion);
	TestTrue(TEXT("Future version"), FSaveJsonParser::Parse(TEXT("{\"version\":2}"), Tables.Value()).Error() == ESaveJsonError::UnsupportedVersion);
	TestTrue(TEXT("Missing fields"), FSaveJsonParser::Parse(TEXT("{\"version\":1}"), Tables.Value()).Error() == ESaveJsonError::InvalidField);

	const FString BadType = Text.Value().Replace(TEXT("\"sword\""), TEXT("\"halberd\""));
	TestTrue(TEXT("Unknown weapon type"), FSaveJsonParser::Parse(BadType, Tables.Value()).Error() == ESaveJsonError::UnknownWeaponType);
	const FString BadDifficulty = Text.Value().Replace(TEXT("\"casual\""), TEXT("\"easy\""));
	TestTrue(TEXT("Unknown difficulty"), FSaveJsonParser::Parse(BadDifficulty, Tables.Value()).Error() == ESaveJsonError::InvalidField);
	const FString BadLevel = Text.Value().Replace(TEXT("\"level\": 4"), TEXT("\"level\": 0"));
	TestTrue(TEXT("Roster rejects an invalid unit"), FSaveJsonParser::Parse(BadLevel, Tables.Value()).Error() == ESaveJsonError::InvalidField);

	// Training for a type the weapon file has no name for cannot be written.
	FSaveData Untyped = Data;
	Untyped.Roster = Roster();
	BattleUnit Odd = FullUnit();
	Odd.WeaponPoints[8] = 10;
	Untyped.Roster.Add(Odd);
	TestTrue(TEXT("Unnamed type cannot be saved"), FSaveJsonParser::Serialize(Untyped, Tables.Value()).Error() == ESaveJsonError::UnknownWeaponType);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCommanderSaveStorageTest,
	"Commander3D.Save.StorageRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCommanderSaveStorageTest::RunTest(const FString& Parameters)
{
	const FString SaveDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("AutomationSaves"));
	IFileManager::Get().DeleteDirectory(*SaveDir, false, true);

	const auto Loaded = FCampaignLoader::Load(DataDir(), TEXT("test_campaign"));
	if (!TestTrue(TEXT("Campaign loads"), Loaded.IsOk()))
	{
		return false;
	}
	// Chapter 0 (not started) and chapter 1 (the one-chapter campaign cleared).
	for (const size_t Index : {static_cast<size_t>(0), static_cast<size_t>(1)})
	{
		commander::application::Campaign Run(Loaded.Value().InitialRoster, Difficulty::Casual, Loaded.Value().Chapters, Index);
		TestTrue(TEXT("Save succeeds"), FSaveStorage::Save(DataDir(), SaveDir, TEXT("slot1"), TEXT("test_campaign"), Run) == ESaveError::None);
		const auto Restored = FSaveStorage::Load(DataDir(), SaveDir, TEXT("slot1"));
		if (!TestTrue(TEXT("Load succeeds"), Restored.IsOk()))
		{
			continue;
		}
		TestEqual(TEXT("Chapter index restored"), static_cast<int32>(Restored.Value().ChapterIndex()), static_cast<int32>(Index));
		TestTrue(TEXT("Difficulty restored"), Restored.Value().Difficulty() == Difficulty::Casual);
		TestEqual(TEXT("Roster size restored"), static_cast<int32>(Restored.Value().Roster().Members().size()), 2);
		TestEqual(TEXT("Gold restored"), Restored.Value().Roster().Gold(), 1000);
		TestTrue(TEXT("No temp file left"), !IFileManager::Get().FileExists(*FPaths::Combine(SaveDir, TEXT("slot1.json.tmp"))));
	}

	TestTrue(TEXT("Missing slot"), FSaveStorage::Load(DataDir(), SaveDir, TEXT("nothing")).Error() == ESaveError::FileNotReadable);
	TestTrue(TEXT("Unsafe slot"), FSaveStorage::Load(DataDir(), SaveDir, TEXT("../x")).Error() == ESaveError::InvalidName);

	// A chapter index past the campaign is rejected on load (slot1 holds chapter 1 now).
	FString Text;
	FFileHelper::LoadFileToString(Text, *FPaths::Combine(SaveDir, TEXT("slot1.json")));
	FFileHelper::SaveStringToFile(Text.Replace(TEXT("\"chapter\": 1"), TEXT("\"chapter\": 5")), *FPaths::Combine(SaveDir, TEXT("far.json")));
	TestTrue(TEXT("Chapter beyond the campaign"), FSaveStorage::Load(DataDir(), SaveDir, TEXT("far")).Error() == ESaveError::InvalidSave);

	IFileManager::Get().DeleteDirectory(*SaveDir, false, true);
	return true;
}
