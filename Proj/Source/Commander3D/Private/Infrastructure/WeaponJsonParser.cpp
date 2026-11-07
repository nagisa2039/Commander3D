#include "Infrastructure/WeaponJsonParser.h"

#include "Infrastructure/JsonReadHelpers.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
using ParseResult = commander::application::Result<FWeaponCatalog, EWeaponJsonError>;

// Reads the optional "types" list into names -> ids (1-based). False on a bad shape, a duplicate or too many.
bool ReadTypes(const TSharedPtr<FJsonObject>& Root, TMap<FString, int32>& Out)
{
	TArray<FString> Names;
	if (!JsonRead::ReadOptionalStringArrayField(Root, TEXT("types"), Names) || Names.Num() > commander::domain::MaxWeaponTypes)
	{
		return false;
	}
	for (int32 I = 0; I < Names.Num(); ++I)
	{
		if (Out.Contains(Names[I]))
		{
			return false;
		}
		Out.Add(Names[I], I + 1);
	}
	return true;
}
} // namespace

commander::application::Result<FWeaponCatalog, EWeaponJsonError> FWeaponJsonParser::Parse(const FString& JsonText)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return ParseResult::Err(EWeaponJsonError::InvalidJson);
	}

	FWeaponCatalog Catalog;
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!ReadTypes(Root, Catalog.Types) || !Root->TryGetArrayField(TEXT("weapons"), Entries))
	{
		return ParseResult::Err(EWeaponJsonError::InvalidField);
	}

	for (const TSharedPtr<FJsonValue>& Entry : *Entries)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		FString Id;
		FString TypeName;
		int32 Price = 0;
		commander::domain::WeaponSpec Spec;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object) || !JsonRead::ReadStringField(*Object, TEXT("id"), Id)
			|| !JsonRead::ReadInt32Field(*Object, TEXT("might"), Spec.Might) || !JsonRead::ReadInt32Field(*Object, TEXT("hit"), Spec.Hit)
			|| !JsonRead::ReadInt32Field(*Object, TEXT("minRange"), Spec.MinRange)
			|| !JsonRead::ReadInt32Field(*Object, TEXT("maxRange"), Spec.MaxRange)
			|| !JsonRead::ReadOptionalInt32Field(*Object, TEXT("durability"), Spec.Durability)
			|| !JsonRead::ReadOptionalInt32Field(*Object, TEXT("rank"), Spec.RequiredRank)
			|| !JsonRead::ReadOptionalInt32Field(*Object, TEXT("avoid"), Spec.Avoid)
			|| !JsonRead::ReadOptionalInt32Field(*Object, TEXT("rankHit"), Spec.RankHit)
			|| !JsonRead::ReadOptionalInt32Field(*Object, TEXT("rankAvoid"), Spec.RankAvoid)
			|| !JsonRead::ReadOptionalInt32Field(*Object, TEXT("price"), Price) || !JsonRead::ReadOptionalStringField(*Object, TEXT("type"), TypeName)
			|| Catalog.Weapons.Contains(Id))
		{
			return ParseResult::Err(EWeaponJsonError::InvalidField);
		}
		if (!TypeName.IsEmpty() || (*Object)->HasField(TEXT("type")))
		{
			const int32* TypeId = Catalog.Types.Find(TypeName);
			if (TypeId == nullptr)
			{
				return ParseResult::Err(EWeaponJsonError::InvalidField);
			}
			Spec.Type = *TypeId;
		}
		Catalog.Weapons.Add(Id, Spec);
		if (Price != 0)
		{
			Catalog.Prices.Add(Id, Price);
		}
	}
	return ParseResult::Ok(std::move(Catalog));
}
