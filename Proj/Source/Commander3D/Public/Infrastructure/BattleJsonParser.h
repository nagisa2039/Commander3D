#pragma once

#include "CoreMinimal.h"

#include "Battle/BattleSetup.h"
#include "Common/Result.h"
#include "Infrastructure/WeaponJsonParser.h"

enum class EBattleJsonError : uint8
{
	None,
	InvalidJson,   // not parseable JSON, or the root is not an object
	InvalidField,  // a required field is missing or has the wrong type / range
	UnknownWeapon, // a unit refers to a weapon id that is not in the weapon table
};

// A parsed battle file: which map to use, and the setup with everything except the map data.
struct FBattleDefinition
{
	FString MapName;
	commander::application::BattleSetup Setup; // Setup.Map is left empty; the loader fills it from MapName.
};

// Infrastructure adapter: battle JSON text -> FBattleDefinition.
//
// {
//   "map": "test_map",                                                        // file name under Data/Maps/ without ".json"
//   "conditions": { "routEnemies": true, "defeatBoss": false, "turnLimit": 0 }, // optional; each key optional
//   "units": [ { "id": 1, "side": "player"|"enemy", "x": 0, "y": 0, "movement": 5, "hp": 18,
//                "strength": 5, "skill": 4, "speed": 6, "defense": 2,
//                "weapon": "iron_sword",   // optional; absent = unarmed
//                "boss": false } ]         // optional
// }
//
// "hp" sets both Hp and MaxHp. Game rules (positions, both sides present, boss present) are checked by StartBattle.
class COMMANDER3D_API FBattleJsonParser
{
public:
	static commander::application::Result<FBattleDefinition, EBattleJsonError> Parse(const FString& JsonText, const FWeaponTable& Weapons);
};
