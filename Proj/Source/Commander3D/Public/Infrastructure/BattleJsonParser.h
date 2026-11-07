#pragma once

#include "CoreMinimal.h"

#include "Dom/JsonObject.h"

#include "Battle/BattleSetup.h"
#include "Common/Result.h"
#include "Infrastructure/ClassJsonParser.h"
#include "Infrastructure/ItemJsonParser.h"
#include "Infrastructure/WeaponJsonParser.h"

enum class EBattleJsonError : uint8
{
	None,
	InvalidJson,   // not parseable JSON, or the root is not an object
	InvalidField,  // a required field is missing or has the wrong type / range
	UnknownWeapon, // a unit's "weapon" is not in the weapon table
	UnknownClass,  // a unit's "class" is not in the class table
	UnknownItem,   // an entry of a unit's "items" or a shop's "stock" is in neither the weapon table nor the item table
	UnknownWeaponType, // a unit's "weaponRanks" names a type the weapon file does not define
};

// The master data a battle file refers to by id.
struct FBattleDataTables
{
	FWeaponCatalog Weapons;
	FItemTable Items;
	FClassTable Classes;
};

// A parsed battle file: which map to use, and the setup with everything except the map data.
struct FBattleDefinition
{
	FString MapName;
	FString Name;                                       // optional "name" (chapter title); empty if absent
	TArray<commander::domain::GridCoord> StartCells;    // optional "startCells": where a chapter's sortie members start
	commander::application::BattleSetup Setup; // Setup.Map is left empty; the loader fills it from MapName.
};

// Infrastructure adapter: battle JSON text -> FBattleDefinition.
//
// {
//   "map": "test_map",                                                        // file name under Data/Maps/ without ".json"
//   "conditions": { "routEnemies": true, "defeatBoss": false, "turnLimit": 0 }, // optional; each key optional
//   "units": [ { "id": 1, "side": "player"|"enemy", "x": 0, "y": 0, "hp": 18,
//                "strength": 5, "skill": 4, "speed": 6, "defense": 2,
//                "class": "soldier",        // optional; gives movement and growth rates
//                "movement": 5,             // required without a class; with one it overrides the class
//                "level": 1, "exp": 0,      // optional (defaults 1 and 0)
//                "weapon": "iron_sword",    // optional; absent = unarmed. Equipped, starts fresh
//                "items": ["iron_bow", "vulnerary"],  // optional inventory: weapon ids or item ids
//                "weaponRanks": {"sword": 1},         // optional starting ranks (0..3) by weapon type name
//                "boss": false } ],         // optional
//   "gold": 500,                            // optional: the player army's gold
//   "shops": [ { "x": 2, "y": 1, "stock": ["vulnerary", "iron_sword"] } ],  // optional; ids as in "items"
//   "preparationStock": ["vulnerary", "iron_sword"],   // optional: what the pre-battle preparation shop sells; ids as in "items"
//   "chests": [ { "x": 3, "y": 1, "item": "vulnerary" }, { "x": 4, "y": 2, "gold": 300 } ],   // optional; exactly one of "item" (id as in "items") and "gold" per chest
//   "name": "Chapter 1",                   // optional: chapter title
//   "startCells": [ {"x": 0, "y": 0} ]      // optional: sortie start cells of a chapter, in deploy order
// }
//
// "hp" sets both Hp and MaxHp. Game rules (positions, ranges, both sides present, boss present) are checked
// by StartBattle.
class COMMANDER3D_API FBattleJsonParser
{
public:
	static commander::application::Result<FBattleDefinition, EBattleJsonError> Parse(const FString& JsonText, const FBattleDataTables& Tables);

	// Reads one roster member from a unit object with the same keys as "units" entries, except that "side"
	// (always player), "x" and "y" (not used) are optional. Rule validation is left to Roster::Add.
	static EBattleJsonError ParseRosterUnit(const TSharedPtr<FJsonObject>& Object, const FBattleDataTables& Tables, commander::domain::BattleUnit& Out);
};
