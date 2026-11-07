#include "Infrastructure/BattleDataLoader.h"

#include "Infrastructure/BattleJsonParser.h"
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

commander::application::Result<BattleSetup, EBattleLoadError> FBattleDataLoader::Load(const FString& DataDir, const FString& BattleName)
{
	if (!IsSafeName(BattleName))
	{
		return LoadResult::Err(EBattleLoadError::InvalidName);
	}

	FString WeaponsText;
	FString BattleText;
	if (!FFileHelper::LoadFileToString(WeaponsText, *FPaths::Combine(DataDir, TEXT("Weapons.json")))
		|| !FFileHelper::LoadFileToString(BattleText, *FPaths::Combine(DataDir, TEXT("Battles"), BattleName + TEXT(".json"))))
	{
		return LoadResult::Err(EBattleLoadError::FileNotReadable);
	}

	const auto Weapons = FWeaponJsonParser::Parse(WeaponsText);
	if (!Weapons.IsOk())
	{
		return LoadResult::Err(EBattleLoadError::InvalidWeapons);
	}
	const auto Battle = FBattleJsonParser::Parse(BattleText, Weapons.Value());
	if (!Battle.IsOk() || !IsSafeName(Battle.Value().MapName))
	{
		return LoadResult::Err(EBattleLoadError::InvalidBattle);
	}

	FString MapText;
	if (!FFileHelper::LoadFileToString(MapText, *FPaths::Combine(DataDir, TEXT("Maps"), Battle.Value().MapName + TEXT(".json"))))
	{
		return LoadResult::Err(EBattleLoadError::FileNotReadable);
	}
	const auto Map = FMapJsonParser::Parse(MapText);
	if (!Map.IsOk())
	{
		return LoadResult::Err(EBattleLoadError::InvalidMap);
	}

	BattleSetup Setup = Battle.Value().Setup;
	Setup.Map = Map.Value();
	return LoadResult::Ok(std::move(Setup));
}
