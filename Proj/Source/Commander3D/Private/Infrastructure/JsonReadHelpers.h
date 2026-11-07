#pragma once

#include "CoreMinimal.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// Strict JSON field readers shared by the Infrastructure parsers. Private to the module.
namespace JsonRead
{
// Accepts only JSON numbers that are whole and fit in [Min, Max].
inline bool ToInt(const TSharedPtr<FJsonValue>& Value, int64 Min, int64 Max, int64& Out)
{
	if (!Value.IsValid() || Value->Type != EJson::Number)
	{
		return false;
	}
	const double Number = Value->AsNumber();
	if (Number != FMath::FloorToDouble(Number) || Number < static_cast<double>(Min) || Number > static_cast<double>(Max))
	{
		return false;
	}
	Out = static_cast<int64>(Number);
	return true;
}

inline bool ReadInt32Field(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, int32& Out)
{
	int64 Wide = 0;
	if (!ToInt(Object->TryGetField(Name), MIN_int32, MAX_int32, Wide))
	{
		return false;
	}
	Out = static_cast<int32>(Wide);
	return true;
}

// Reads an integer in [Min, Max].
inline bool ReadIntFieldInRange(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, int64 Min, int64 Max, int64& Out)
{
	return ToInt(Object->TryGetField(Name), Min, Max, Out);
}

// A bool field that may be absent (keeps Out unchanged). False if present with the wrong type.
inline bool ReadOptionalBoolField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, bool& Out)
{
	const TSharedPtr<FJsonValue> Field = Object->TryGetField(Name);
	if (!Field.IsValid())
	{
		return true;
	}
	// TryGetBool alone would also accept numbers, so the JSON type is checked explicitly.
	return Field->Type == EJson::Boolean && Field->TryGetBool(Out);
}

// A string field that must be present.
inline bool ReadStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, FString& Out)
{
	return Object->TryGetStringField(Name, Out);
}
} // namespace JsonRead
