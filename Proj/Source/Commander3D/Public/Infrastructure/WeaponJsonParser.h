#pragma once

#include "CoreMinimal.h"

#include "Battle/BattleUnit.h"
#include "Common/Result.h"

enum class EWeaponJsonError : uint8
{
	None,
	InvalidJson,  // not parseable JSON, or the root is not an object
	InvalidField, // a required field is missing / has the wrong type or range, an unknown type name, or a duplicated id
};

// Weapon id -> numbers.
using FWeaponTable = TMap<FString, commander::domain::WeaponSpec>;

// Everything the weapon file defines.
struct FWeaponCatalog
{
	FWeaponTable Weapons;
	TMap<FString, int32> Types;  // weapon type name -> type id (1..MaxWeaponTypes, in file order)
	TMap<FString, int32> Prices; // weapon id -> buying price (absent = 0, not for sale)
};

// Infrastructure adapter: weapons JSON text -> catalog.
//
// { "types": ["sword", "lance", "bow"],                       // optional; type id = position + 1 (at most 8)
//   "weapons": [ {"id": "iron_sword", "might": 5, "hit": 90, "minRange": 1, "maxRange": 1,
//                 "durability": 46,                            // optional; absent (0) = never breaks
//                 "type": "sword", "rank": 0,                  // optional; "type" must be listed in "types"; rank = rank needed
//                 "avoid": 0, "rankHit": 0, "rankAvoid": 0,    // optional (default 0): avoid for the wielder, hit / avoid per rank
//                 "price": 460}, ... ] }                       // optional
//
// Whether the numbers make sense (range order, hit 0..100, durability >= 0, rank 0..3, avoid -100..100,
// rankHit / rankAvoid 0..50) is checked by the Domain when a unit is added.
class COMMANDER3D_API FWeaponJsonParser
{
public:
	static commander::application::Result<FWeaponCatalog, EWeaponJsonError> Parse(const FString& JsonText);
};
