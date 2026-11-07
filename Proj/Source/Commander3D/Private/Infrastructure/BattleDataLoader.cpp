#include "Infrastructure/BattleDataLoader.h"

#include "Infrastructure/BattleJsonParser.h"
#include "Infrastructure/ClassJsonParser.h"
#include "Infrastructure/ItemJsonParser.h"
#include "Infrastructure/MapJsonParser.h"
#include "Infrastructure/WeaponJsonParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
using commander::application::BattleSetup;
using LoadResult = commander::application::Result<BattleSetup, EBattleLoadError>;

// Names become file names, so only a safe character set is allowed (no separators, no "..").
bool IsSafeName(const FString& Name)
{
	if (Name.IsEmpty())
	{
		return false;
	}
	for (const TCHAR C : Name)
	{
		if (!FChar::IsAlnum(C) && C != TEXT('_') && C != TEXT('-'))
		{
			return false;
		}
	}
	return true;
}
} // namespace

commander::application::Result<FBattleDataTables, EBattleLoadError> FBattleDataLoader::LoadTables(const FString& DataDir)
{
	using TablesResult = commander::application::Result<FBattleDataTables, EBattleLoadError>;
	FString WeaponsText;
	FString ItemsText;
	FString ClassesText;
	if (!FFileHelper::LoadFileToString(WeaponsText, *FPaths::Combine(DataDir, TEXT("Weapons.json")))
		|| !FFileHelper::LoadFileToString(ItemsText, *FPaths::Combine(DataDir, TEXT("Items.json")))
		|| !FFileHelper::LoadFileToString(ClassesText, *FPaths::Combine(DataDir, TEXT("Classes.json"))))
	{
		return TablesResult::Err(EBattleLoadError::FileNotReadable);
	}

	const auto Weapons = FWeaponJsonParser::Parse(WeaponsText);
	if (!Weapons.IsOk())
	{
		return TablesResult::Err(EBattleLoadError::InvalidWeapons);
	}
	const auto Items = FItemJsonParser::Parse(ItemsText);
	if (!Items.IsOk())
	{
		return TablesResult::Err(EBattleLoadError::InvalidItems);
	}
	const auto Classes = FClassJsonParser::Parse(ClassesText);
	if (!Classes.IsOk())
	{
		return TablesResult::Err(EBattleLoadError::InvalidClasses);
	}
	FBattleDataTables Tables;
	Tables.Weapons = Weapons.Value(); // the whole catalog: weapons, type ids, prices
	Tables.Items = Items.Value();
	Tables.Classes = Classes.Value();
	return TablesResult::Ok(std::move(Tables));
}

commander::application::Result<FBattleDefinition, EBattleLoadError> FBattleDataLoader::LoadDefinition(
	const FString& DataDir, const FBattleDataTables& Tables, const FString& SubDir, const FString& Name)
{
	using DefinitionResult = commander::application::Result<FBattleDefinition, EBattleLoadError>;
	if (!IsSafeName(SubDir) || !IsSafeName(Name))
	{
		return DefinitionResult::Err(EBattleLoadError::InvalidName);
	}

	FString BattleText;
	if (!FFileHelper::LoadFileToString(BattleText, *FPaths::Combine(DataDir, SubDir, Name + TEXT(".json"))))
	{
		return DefinitionResult::Err(EBattleLoadError::FileNotReadable);
	}
	const auto Battle = FBattleJsonParser::Parse(BattleText, Tables);
	if (!Battle.IsOk() || !IsSafeName(Battle.Value().MapName))
	{
		return DefinitionResult::Err(EBattleLoadError::InvalidBattle);
	}

	FString MapText;
	if (!FFileHelper::LoadFileToString(MapText, *FPaths::Combine(DataDir, TEXT("Maps"), Battle.Value().MapName + TEXT(".json"))))
	{
		return DefinitionResult::Err(EBattleLoadError::FileNotReadable);
	}
	const auto Map = FMapJsonParser::Parse(MapText);
	if (!Map.IsOk())
	{
		return DefinitionResult::Err(EBattleLoadError::InvalidMap);
	}

	FBattleDefinition Definition = Battle.Value();
	Definition.Setup.Map = Map.Value();
	return DefinitionResult::Ok(std::move(Definition));
}

commander::application::Result<BattleSetup, EBattleLoadError> FBattleDataLoader::Load(const FString& DataDir, const FString& BattleName)
{
	if (!IsSafeName(BattleName))
	{
		return LoadResult::Err(EBattleLoadError::InvalidName);
	}
	const auto Tables = LoadTables(DataDir);
	if (!Tables.IsOk())
	{
		return LoadResult::Err(Tables.Error());
	}
	const auto Definition = LoadDefinition(DataDir, Tables.Value(), TEXT("Battles"), BattleName);
	if (!Definition.IsOk())
	{
		return LoadResult::Err(Definition.Error());
	}
	return LoadResult::Ok(Definition.Value().Setup);
}
