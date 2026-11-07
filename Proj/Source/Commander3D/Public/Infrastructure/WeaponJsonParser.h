#pragma once

#include "CoreMinimal.h"

#include "Battle/BattleUnit.h"
#include "Common/Result.h"

enum class EWeaponJsonError : uint8
{
	None,
	InvalidJson,  // not parseable JSON, or the root is not an object
	InvalidField, // a required field is missing / has the wrong type or range, or an id is duplicated
};

// Weapon id -> numbers.
using FWeaponTable = TMap<FString, commander::domain::WeaponSpec>;

// Infrastructure adapter: weapons JSON text -> weapon table.
//
// { "weapons": [ {"id": "iron_sword", "might": 5, "hit": 90, "minRange": 1, "maxRange": 1}, ... ] }
//
// Whether the numbers make sense (range order, hit 0..100) is checked by the Domain when a unit is added.
class COMMANDER3D_API FWeaponJsonParser
{
public:
	static commander::application::Result<FWeaponTable, EWeaponJsonError> Parse(const FString& JsonText);
};
