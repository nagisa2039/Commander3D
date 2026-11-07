#include "Infrastructure/SaveStorage.h"

#include "HAL/FileManager.h"
#include "Infrastructure/BattleDataLoader.h"
#include "Infrastructure/CampaignLoader.h"
#include "Infrastructure/SaveJsonParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
using commander::application::Campaign;
using LoadResult = commander::application::Result<Campaign, ESaveError>;

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

ESaveError FSaveStorage::Save(const FString& DataDir, const FString& SaveDir, const FString& Slot, const FString& CampaignName, const Campaign& Run)
{
	if (!IsSafeName(Slot) || !IsSafeName(CampaignName))
	{
		return ESaveError::InvalidName;
	}
	const auto Tables = FBattleDataLoader::LoadTables(DataDir);
	if (!Tables.IsOk())
	{
		return ESaveError::InvalidData;
	}

	FSaveData Data;
	Data.CampaignName = CampaignName;
	Data.Difficulty = Run.Difficulty();
	Data.ChapterIndex = static_cast<int32>(Run.ChapterIndex());
	Data.Roster = Run.Roster();
	const auto Text = FSaveJsonParser::Serialize(Data, Tables.Value());
	if (!Text.IsOk())
	{
		return ESaveError::InvalidData;
	}

	IFileManager::Get().MakeDirectory(*SaveDir, true);

	// Write to a temp file first so a failed write cannot destroy the previous save.
	const FString Path = FPaths::Combine(SaveDir, Slot + TEXT(".json"));
	const FString TempPath = Path + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(Text.Value(), *TempPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
		|| !IFileManager::Get().Move(*Path, *TempPath, /*Replace=*/true))
	{
		IFileManager::Get().Delete(*TempPath);
		return ESaveError::FileNotReadable;
	}
	return ESaveError::None;
}

commander::application::Result<Campaign, ESaveError> FSaveStorage::Load(const FString& DataDir, const FString& SaveDir, const FString& Slot)
{
	if (!IsSafeName(Slot))
	{
		return LoadResult::Err(ESaveError::InvalidName);
	}
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(SaveDir, Slot + TEXT(".json"))))
	{
		return LoadResult::Err(ESaveError::FileNotReadable);
	}
	const auto Tables = FBattleDataLoader::LoadTables(DataDir);
	if (!Tables.IsOk())
	{
		return LoadResult::Err(ESaveError::InvalidData);
	}
	const auto Data = FSaveJsonParser::Parse(Text, Tables.Value());
	if (!Data.IsOk() || !IsSafeName(Data.Value().CampaignName))
	{
		return LoadResult::Err(ESaveError::InvalidSave);
	}
	const auto Definition = FCampaignLoader::Load(DataDir, Data.Value().CampaignName);
	if (!Definition.IsOk())
	{
		return LoadResult::Err(ESaveError::InvalidData);
	}
	if (static_cast<size_t>(Data.Value().ChapterIndex) > Definition.Value().Chapters.size())
	{
		return LoadResult::Err(ESaveError::InvalidSave);
	}
	return LoadResult::Ok(Campaign(Data.Value().Roster, Data.Value().Difficulty, Definition.Value().Chapters,
		static_cast<size_t>(Data.Value().ChapterIndex)));
}
