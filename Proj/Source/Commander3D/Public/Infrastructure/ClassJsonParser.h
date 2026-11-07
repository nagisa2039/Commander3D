#pragma once

#include "CoreMinimal.h"

#include "Battle/BattleUnit.h"
#include "Common/Result.h"

enum class EClassJsonError : uint8
{
	None,
	InvalidJson,  // not parseable JSON, or the root is not an object
	InvalidField, // a required field is missing / has the wrong type, or an id is duplicated
};

// A class change a class offers: reaching MinLevel lets a unit become TargetClass (which supplies the new
// growth and caps) and gain Bonus.
struct FClassPromotion
{
	int32 MinLevel = 1;
	FString TargetClass;
	commander::domain::StatBonus Bonus;
};

// What a class gives its units: base movement, level-up growth rates, stat caps and the class changes it offers
// (in file order; at most MaxPromotionOptions, each to a different target).
struct FClassSpec
{
	int32 Movement = 0;
	commander::domain::GrowthRates Growth;
	commander::domain::StatCaps Caps;
	TArray<FClassPromotion> Promotions;
};

using FClassTable = TMap<FString, FClassSpec>;

// Infrastructure adapter: classes JSON text -> class table.
//
// { "classes": [ {"id": "soldier", "movement": 5,
//                 "growth": {"hp": 60, "strength": 40, "skill": 30, "speed": 30, "defense": 20},
//                 "caps": {"hp": 60, "strength": 30, "skill": 30, "speed": 30, "defense": 30},
//                 "promotions": [ {"level": 10, "to": "knight",
//                                  "bonus": {"hp": 3, "strength": 2, "skill": 1, "speed": 1, "defense": 2, "movement": 1}},
//                                 {"level": 8, "to": "mage"} ]}, ... ] }
//
// "growth", "caps", "promotions" and the keys inside them are optional (default 0 = no growth / no cap / no
// options); each promotion option needs "level" and "to" (its target class, which must differ between options).
// The target class is looked up when a unit is built, not here. Ranges (growth 0..100) are checked by the Domain
// when a unit is added.
class COMMANDER3D_API FClassJsonParser
{
public:
	static commander::application::Result<FClassTable, EClassJsonError> Parse(const FString& JsonText);
};
