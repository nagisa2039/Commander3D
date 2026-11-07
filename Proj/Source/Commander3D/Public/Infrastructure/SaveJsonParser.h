#pragma once

#include "CoreMinimal.h"

#include "Common/Result.h"
#include "Infrastructure/BattleJsonParser.h"
#include "Roster/Roster.h"

enum class ESaveJsonError : uint8
{
	None,
	InvalidJson,        // not parseable JSON, or the root is not an object
	UnsupportedVersion, // "version" is missing or not the supported one
	InvalidField,       // a required field is missing or has the wrong type / range, or the roster rejected a unit
	UnknownWeaponType,  // "weaponPoints" names a weapon type the weapon file does not define (or a trained type has no name)
};

// What a save holds: which campaign, how far, the difficulty and the army. Between battles only.
struct FSaveData
{
	FString CampaignName;
	commander::domain::Difficulty Difficulty = commander::domain::Difficulty::Classic;
	int32 ChapterIndex = 0; // the chapter to play next (the number of chapters cleared)
	commander::domain::Roster Roster;
};

// Infrastructure adapter: FSaveData <-> save JSON text.
//
// { "version": 1, "campaign": "test_campaign", "difficulty": "classic"|"casual", "chapter": 2, "gold": 500,
//   "units": [ { "id": 1, "hp": 18, "movement": 5, "strength": 5, "skill": 4, "speed": 6, "defense": 2,
//                "level": 3, "exp": 40, "boss": false,
//                "weapon": { <weapon>, "uses": 12 },            // equipped; MaxRange 0 = unarmed
//                "growth": {"hp":..,"strength":..,"skill":..,"speed":..,"defense":..}, "caps": { same keys },
//                "promotions": [ {"name":"knight","level":10,"bonus":{ + "movement" },"growth":{..},"caps":{..}} ],
//                "items": [ {"kind":"weapon"|"potion"|"repair"|"staff"|"seal","uses":..,"amount":..,"range":..,"price":..,"weapon":{<weapon>}} ],
//                "weaponPoints": {"sword": 120} } ] }          // training by weapon type name
//
// <weapon> = {"might","hit","minRange","maxRange","durability","type","requiredRank","avoid","rankHit","rankAvoid"} (all numbers).
// Saves are self-contained: weapons and items are written out in full, so later edits of the master data do not
// change a saved army. Only weapon types are looked up by name (their ids depend on the weapon file's order).
// Units are saved as they stand between battles (position and turn state are not kept; Hp is MaxHp).
class COMMANDER3D_API FSaveJsonParser
{
public:
	static commander::application::Result<FString, ESaveJsonError> Serialize(const FSaveData& Data, const FBattleDataTables& Tables);
	static commander::application::Result<FSaveData, ESaveJsonError> Parse(const FString& JsonText, const FBattleDataTables& Tables);
};
