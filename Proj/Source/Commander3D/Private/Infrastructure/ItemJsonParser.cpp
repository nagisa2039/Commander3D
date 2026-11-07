#include "Infrastructure/ItemJsonParser.h"

#include "Infrastructure/JsonReadHelpers.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
using commander::domain::ItemKind;
using ParseResult = commander::application::Result<FItemTable, EItemJsonError>;
} // namespace

commander::application::Result<FItemTable, EItemJsonError> FItemJsonParser::Parse(const FString& JsonText)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return ParseResult::Err(EItemJsonError::InvalidJson);
	}

	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Root->TryGetArrayField(TEXT("items"), Entries))
	{
		return ParseResult::Err(EItemJsonError::InvalidField);
	}

	FItemTable Table;
	for (const TSharedPtr<FJsonValue>& Entry : *Entries)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		FString Id;
		FString Kind;
		commander::domain::ItemSlot Slot;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object) || !JsonRead::ReadStringField(*Object, TEXT("id"), Id)
			|| !JsonRead::ReadStringField(*Object, TEXT("kind"), Kind) || !JsonRead::ReadInt32Field(*Object, TEXT("uses"), Slot.Uses)
			|| !JsonRead::ReadOptionalInt32Field(*Object, TEXT("price"), Slot.Price) || Table.Contains(Id))
		{
			return ParseResult::Err(EItemJsonError::InvalidField);
		}

		if (Kind == TEXT("potion"))
		{
			Slot.Kind = ItemKind::Potion;
			if (!JsonRead::ReadInt32Field(*Object, TEXT("amount"), Slot.Amount))
			{
				return ParseResult::Err(EItemJsonError::InvalidField);
			}
		}
		else if (Kind == TEXT("repair"))
		{
			Slot.Kind = ItemKind::RepairKit;
		}
		else if (Kind == TEXT("staff"))
		{
			Slot.Kind = ItemKind::Staff;
			if (!JsonRead::ReadInt32Field(*Object, TEXT("amount"), Slot.Amount) || !JsonRead::ReadInt32Field(*Object, TEXT("range"), Slot.Range))
			{
				return ParseResult::Err(EItemJsonError::InvalidField);
			}
		}
		else if (Kind == TEXT("seal"))
		{
			// A promotion seal: "level" is the lowest unit level it can be used at (stored as Amount).
			Slot.Kind = ItemKind::PromotionSeal;
			if (!JsonRead::ReadInt32Field(*Object, TEXT("level"), Slot.Amount))
			{
				return ParseResult::Err(EItemJsonError::InvalidField);
			}
		}
		else
		{
			return ParseResult::Err(EItemJsonError::InvalidField);
		}
		Table.Add(Id, Slot);
	}
	return ParseResult::Ok(std::move(Table));
}
