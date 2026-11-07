#include "Infrastructure/CampaignLoader.h"

#include "Infrastructure/BattleDataLoader.h"
#include "Infrastructure/JsonReadHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
using LoadResult = commander::application::Result<FCampaignDefinition, ECampaignLoadError>;

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

ECampaignLoadError FromLoadError(EBattleLoadError Error)
{
	return Error == EBattleLoadError::FileNotReadable ? ECampaignLoadError::FileNotReadable : ECampaignLoadError::InvalidChapter;
}
} // namespace

commander::application::Result<FCampaignDefinition, ECampaignLoadError> FCampaignLoader::Load(const FString& DataDir, const FString& CampaignName)
{
	if (!IsSafeName(CampaignName))
	{
		return LoadResult::Err(ECampaignLoadError::InvalidName);
	}

	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(DataDir, TEXT("Campaigns"), CampaignName + TEXT(".json"))))
	{
		return LoadResult::Err(ECampaignLoadError::FileNotReadable);
	}
	const auto Tables = FBattleDataLoader::LoadTables(DataDir);
	if (!Tables.IsOk())
	{
		return LoadResult::Err(Tables.Error() == EBattleLoadError::FileNotReadable ? ECampaignLoadError::FileNotReadable : ECampaignLoadError::InvalidData);
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	const TArray<TSharedPtr<FJsonValue>>* RosterEntries = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* ChapterEntries = nullptr;
	int32 Gold = 0;
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid() || !Root->TryGetArrayField(TEXT("roster"), RosterEntries)
		|| !Root->TryGetArrayField(TEXT("chapters"), ChapterEntries) || ChapterEntries->IsEmpty()
		|| !JsonRead::ReadOptionalInt32Field(Root, TEXT("gold"), Gold))
	{
		return LoadResult::Err(ECampaignLoadError::InvalidCampaign);
	}

	FCampaignDefinition Definition;
	if (!Definition.InitialRoster.SetGold(Gold))
	{
		return LoadResult::Err(ECampaignLoadError::InvalidCampaign);
	}
	for (const TSharedPtr<FJsonValue>& Entry : *RosterEntries)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		commander::domain::BattleUnit Unit;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object)
			|| FBattleJsonParser::ParseRosterUnit(*Object, Tables.Value(), Unit) != EBattleJsonError::None || !Definition.InitialRoster.Add(Unit))
		{
			return LoadResult::Err(ECampaignLoadError::InvalidRoster);
		}
	}

	for (const TSharedPtr<FJsonValue>& Entry : *ChapterEntries)
	{
		FString ChapterId;
		if (!Entry.IsValid() || Entry->Type != EJson::String || !Entry->TryGetString(ChapterId))
		{
			return LoadResult::Err(ECampaignLoadError::InvalidCampaign);
		}
		if (!IsSafeName(ChapterId))
		{
			return LoadResult::Err(ECampaignLoadError::InvalidName);
		}
		const auto Loaded = FBattleDataLoader::LoadDefinition(DataDir, Tables.Value(), TEXT("Chapters"), ChapterId);
		if (!Loaded.IsOk())
		{
			return LoadResult::Err(FromLoadError(Loaded.Error()));
		}
		if (Loaded.Value().StartCells.IsEmpty())
		{
			return LoadResult::Err(ECampaignLoadError::InvalidChapter);
		}

		commander::application::Chapter Chapter;
		Chapter.Name = TCHAR_TO_UTF8(Loaded.Value().Name.IsEmpty() ? *ChapterId : *Loaded.Value().Name);
		Chapter.Setup = Loaded.Value().Setup;
		for (const commander::domain::GridCoord& Cell : Loaded.Value().StartCells)
		{
			Chapter.StartCells.push_back(Cell);
		}
		Definition.Chapters.push_back(std::move(Chapter));
	}
	return LoadResult::Ok(std::move(Definition));
}
