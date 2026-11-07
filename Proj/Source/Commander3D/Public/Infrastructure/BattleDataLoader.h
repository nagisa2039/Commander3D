#pragma once

#include "CoreMinimal.h"

#include "Battle/BattleSetup.h"
#include "Common/Result.h"

enum class EBattleLoadError : uint8
{
	None,
	InvalidName,     // the battle name is empty or has characters other than letters, digits, '_' and '-'
	FileNotReadable, // Weapons.json, Items.json, Classes.json, the battle file or the map file could not be read
	InvalidWeapons,  // Weapons.json did not parse
	InvalidItems,    // Items.json did not parse
	InvalidClasses,  // Classes.json did not parse
	InvalidBattle,   // the battle file did not parse (including an unknown weapon id) or its map name is not a valid name
	InvalidMap,      // the map file did not parse
};

// Infrastructure: reads <DataDir>/Weapons.json, Items.json, Classes.json, <DataDir>/Battles/<BattleName>.json and the map it names
// (<DataDir>/Maps/<map>.json) and assembles a BattleSetup. Rule validation is left to StartBattle / LoadMap.
class COMMANDER3D_API FBattleDataLoader
{
public:
	static commander::application::Result<commander::application::BattleSetup, EBattleLoadError> Load(
		const FString& DataDir, const FString& BattleName);
};
