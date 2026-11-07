#pragma once

#include "CoreMinimal.h"

#include "Battle/Campaign.h"
#include "Common/Result.h"

enum class ESaveError : uint8
{
	None,
	InvalidName,     // the slot or campaign name is not file-name safe
	FileNotReadable, // the slot file (or master data) could not be read / written
	InvalidData,     // master data or the campaign did not load
	InvalidSave,     // the save did not parse, or its chapter is beyond the campaign
};

// Infrastructure: saves a campaign between battles to <SaveDir>/<Slot>.json and restores it.
// The campaign's chapters and master data are read again from <DataDir> on load; only the progress is in the save.
class COMMANDER3D_API FSaveStorage
{
public:
	// Writes the campaign's roster (including preparation changes made so far), difficulty and chapter index.
	static ESaveError Save(const FString& DataDir, const FString& SaveDir, const FString& Slot, const FString& CampaignName,
		const commander::application::Campaign& Run);

	// OutCampaignName (optional) receives the name of the campaign the save belongs to.
	static commander::application::Result<commander::application::Campaign, ESaveError> Load(
		const FString& DataDir, const FString& SaveDir, const FString& Slot, FString* OutCampaignName = nullptr);
};
