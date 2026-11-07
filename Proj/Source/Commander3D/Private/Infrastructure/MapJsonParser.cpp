#include "Infrastructure/MapJsonParser.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
using commander::application::MapData;
using commander::application::Result;
using ParseResult = Result<MapData, EMapJsonError>;

// Accepts only JSON numbers that are whole and fit in [Min, Max].
bool ToInt(const TSharedPtr<FJsonValue>& Value, int64 Min, int64 Max, int64& Out)
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

bool ReadInt32Field(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, int32& Out)
{
	int64 Wide = 0;
	if (!ToInt(Object->TryGetField(Name), MIN_int32, MAX_int32, Wide))
	{
		return false;
	}
	Out = static_cast<int32>(Wide);
	return true;
}
} // namespace

commander::application::Result<commander::application::MapData, EMapJsonError> FMapJsonParser::Parse(const FString& JsonText)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return ParseResult::Err(EMapJsonError::InvalidJson);
	}

	MapData Data;
	if (!ReadInt32Field(Root, TEXT("width"), Data.Width) || !ReadInt32Field(Root, TEXT("height"), Data.Height))
	{
		return ParseResult::Err(EMapJsonError::InvalidField);
	}

	const TArray<TSharedPtr<FJsonValue>>* Terrains = nullptr;
	if (!Root->TryGetArrayField(TEXT("terrains"), Terrains))
	{
		return ParseResult::Err(EMapJsonError::InvalidField);
	}
	for (const TSharedPtr<FJsonValue>& Entry : *Terrains)
	{
		const TSharedPtr<FJsonObject>* TerrainObject = nullptr;
		commander::domain::TerrainSpec Spec;
		if (!Entry.IsValid() || !Entry->TryGetObject(TerrainObject) || !ReadInt32Field(*TerrainObject, TEXT("moveCost"), Spec.MoveCost)
			|| !(*TerrainObject)->TryGetBoolField(TEXT("passable"), Spec.Passable))
		{
			return ParseResult::Err(EMapJsonError::InvalidField);
		}
		Data.Terrains.push_back(Spec);
	}

	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!Root->TryGetArrayField(TEXT("cells"), Rows))
	{
		return ParseResult::Err(EMapJsonError::InvalidField);
	}
	TArray<const TArray<TSharedPtr<FJsonValue>>*> RowArrays;
	for (const TSharedPtr<FJsonValue>& Row : *Rows)
	{
		const TArray<TSharedPtr<FJsonValue>>* Cells = nullptr;
		if (!Row.IsValid() || !Row->TryGetArray(Cells))
		{
			return ParseResult::Err(EMapJsonError::InvalidField);
		}
		for (const TSharedPtr<FJsonValue>& Cell : *Cells)
		{
			int64 Id = 0;
			if (!ToInt(Cell, 0, MAX_uint16, Id))
			{
				return ParseResult::Err(EMapJsonError::InvalidField);
			}
			Data.Cells.push_back(static_cast<commander::domain::TerrainId>(Id));
		}
		RowArrays.Add(Cells);
	}

	// Shape is checked after all fields are known to be well-formed.
	bool bShapeOk = RowArrays.Num() == Data.Height;
	for (const TArray<TSharedPtr<FJsonValue>>* Cells : RowArrays)
	{
		bShapeOk = bShapeOk && Cells->Num() == Data.Width;
	}
	if (!bShapeOk)
	{
		return ParseResult::Err(EMapJsonError::RowShapeMismatch);
	}
	return ParseResult::Ok(std::move(Data));
}
