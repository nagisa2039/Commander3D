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

// An int32 field that may be absent (keeps Out unchanged). False if present but not a whole int32.
inline bool ReadOptionalInt32Field(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, int32& Out)
{
	return !Object->HasField(Name) || ReadInt32Field(Object, Name, Out);
}

// Reads an integer in [Min, Max].
inline bool ReadIntFieldInRange(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, int64 Min, int64 Max, int64& Out)
{
	return ToInt(Object->TryGetField(Name), Min, Max, Out);
}

// A bool field that must be present. Numbers, strings and null are rejected (TryGetBool would convert numbers).
inline bool ReadBoolField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, bool& Out)
{
	const TSharedPtr<FJsonValue> Field = Object->TryGetField(Name);
	return Field.IsValid() && Field->Type == EJson::Boolean && Field->TryGetBool(Out);
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

// A string field that must be present. Numbers and bools are rejected (TryGetStringField would convert them).
inline bool ReadStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, FString& Out)
{
	const TSharedPtr<FJsonValue> Field = Object->TryGetField(Name);
	return Field.IsValid() && Field->Type == EJson::String && Field->TryGetString(Out);
}

// A string field that may be absent (keeps Out unchanged). False if present but not a string.
inline bool ReadOptionalStringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, FString& Out)
{
	return !Object->HasField(Name) || ReadStringField(Object, Name, Out);
}

// An array of strings that may be absent. False if present but not an array, or if an element is not a string.
inline bool ReadOptionalStringArrayField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, TArray<FString>& Out)
{
	if (!Object->HasField(Name))
	{
		return true;
	}
	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (!Object->TryGetArrayField(Name, Values))
	{
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Values)
	{
		FString Text;
		if (!Value.IsValid() || Value->Type != EJson::String || !Value->TryGetString(Text))
		{
			return false;
		}
		Out.Add(Text);
	}
	return true;
}
} // namespace JsonRead
