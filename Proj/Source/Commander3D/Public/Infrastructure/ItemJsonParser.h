#pragma once

#include "CoreMinimal.h"

#include "Battle/BattleUnit.h"
#include "Common/Result.h"

enum class EItemJsonError : uint8
{
	None,
	InvalidJson,  // not parseable JSON, or the root is not an object
	InvalidField, // a required field is missing / has the wrong type, an unknown kind, or a duplicated id
};

// Item id -> inventory slot template (a fresh item). Spare weapons are not listed here; they come from the
// weapon table.
using FItemTable = TMap<FString, commander::domain::ItemSlot>;

// Infrastructure adapter: consumable items JSON text -> item table.
//
// { "items": [ {"id": "vulnerary", "kind": "potion", "uses": 3, "amount": 10, "price": 300},  // "amount" is required for potions
//              {"id": "repair_kit", "kind": "repair", "uses": 1},
//              {"id": "heal_staff", "kind": "staff", "uses": 5, "amount": 12, "range": 3},    // staffs also need "range"
//              {"id": "master_seal", "kind": "seal", "uses": 1, "level": 5},                  // promotion seal: lowest usable level
//              ... ] }
//
// "price" is optional (absent = 0, not for sale). Ranges (uses > 0, amount > 0, range >= 1, seal level 1..20,
// price >= 0) are checked by the Domain when a unit is added.
class COMMANDER3D_API FItemJsonParser
{
public:
	static commander::application::Result<FItemTable, EItemJsonError> Parse(const FString& JsonText);
};
