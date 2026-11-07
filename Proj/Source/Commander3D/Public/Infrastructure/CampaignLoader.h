#pragma once

#include "CoreMinimal.h"

#include <vector>

#include "Battle/Campaign.h"
#include "Common/Result.h"
#include "Roster/Roster.h"

enum class ECampaignLoadError : uint8
{
	None,
	InvalidName,     // a campaign or chapter name is empty or has characters other than letters, digits, '_' and '-'
	FileNotReadable, // the campaign, a chapter or its map file could not be read (or a master data file)
	InvalidData,     // Weapons.json, Items.json or Classes.json did not parse
	InvalidCampaign, // the campaign file did not parse, or lists no chapter
	InvalidRoster,   // a roster entry did not parse, or the roster rejected it (invalid data, duplicate id)
	InvalidChapter,  // a chapter file or its map did not parse, or it has no "startCells"
};

// What a campaign file defines: the starting roster and the chapters in order.
struct FCampaignDefinition
{
	commander::domain::Roster InitialRoster;
	std::vector<commander::application::Chapter> Chapters;
};

// Infrastructure: <DataDir>/Campaigns/<CampaignName>.json
//
// { "gold": 500,                              // optional: the army's starting gold
//   "roster": [ { "id": 1, "class": "soldier", "hp": 18, ... } ],   // unit objects as in a battle file's "units"; "side", "x", "y" optional
//   "chapters": ["ch01", "ch02"] }            // file names under <DataDir>/Chapters/ (battle files with "startCells")
//
// Chapters are battle files (enemies, map, conditions, shops) that also give "name" and "startCells".
class COMMANDER3D_API FCampaignLoader
{
public:
	static commander::application::Result<FCampaignDefinition, ECampaignLoadError> Load(const FString& DataDir, const FString& CampaignName);
};
