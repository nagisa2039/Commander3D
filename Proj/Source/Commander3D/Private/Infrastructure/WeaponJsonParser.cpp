#include "Infrastructure/WeaponJsonParser.h"

#include "Infrastructure/JsonReadHelpers.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
using ParseResult = commander::application::Result<FWeaponTable, EWeaponJsonError>;
} // namespace

commander::application::Result<FWeaponTable, EWeaponJsonError> FWeaponJsonParser::Parse(const FString& JsonText)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return ParseResult::Err(EWeaponJsonError::InvalidJson);
	}

	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Root->TryGetArrayField(TEXT("weapons"), Entries))
	{
		return ParseResult::Err(EWeaponJsonError::InvalidField);
	}

	FWeaponTable Table;
	for (const TSharedPtr<FJsonValue>& Entry : *Entries)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		FString Id;
		commander::domain::WeaponSpec Spec;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object) || !JsonRead::ReadStringField(*Object, TEXT("id"), Id)
			|| !JsonRead::ReadInt32Field(*Object, TEXT("might"), Spec.Might) || !JsonRead::ReadInt32Field(*Object, TEXT("hit"), Spec.Hit)
			|| !JsonRead::ReadInt32Field(*Object, TEXT("minRange"), Spec.MinRange)
			|| !JsonRead::ReadInt32Field(*Object, TEXT("maxRange"), Spec.MaxRange) || Table.Contains(Id))
		{
			return ParseResult::Err(EWeaponJsonError::InvalidField);
		}
		Table.Add(Id, Spec);
	}
	return ParseResult::Ok(std::move(Table));
}
