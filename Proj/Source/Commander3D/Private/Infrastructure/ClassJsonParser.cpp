#include "Infrastructure/ClassJsonParser.h"

#include "Infrastructure/JsonReadHelpers.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
using ParseResult = commander::application::Result<FClassTable, EClassJsonError>;

bool ReadGrowth(const TSharedPtr<FJsonObject>& ClassObject, commander::domain::GrowthRates& Out)
{
	if (!ClassObject->HasField(TEXT("growth")))
	{
		return true;
	}
	const TSharedPtr<FJsonObject>* Growth = nullptr;
	if (!ClassObject->TryGetObjectField(TEXT("growth"), Growth))
	{
		return false;
	}
	return JsonRead::ReadOptionalInt32Field(*Growth, TEXT("hp"), Out.Hp) && JsonRead::ReadOptionalInt32Field(*Growth, TEXT("strength"), Out.Strength)
		&& JsonRead::ReadOptionalInt32Field(*Growth, TEXT("skill"), Out.Skill) && JsonRead::ReadOptionalInt32Field(*Growth, TEXT("speed"), Out.Speed)
		&& JsonRead::ReadOptionalInt32Field(*Growth, TEXT("defense"), Out.Defense);
}

bool ReadCaps(const TSharedPtr<FJsonObject>& ClassObject, commander::domain::StatCaps& Out)
{
	if (!ClassObject->HasField(TEXT("caps")))
	{
		return true;
	}
	const TSharedPtr<FJsonObject>* Caps = nullptr;
	if (!ClassObject->TryGetObjectField(TEXT("caps"), Caps))
	{
		return false;
	}
	return JsonRead::ReadOptionalInt32Field(*Caps, TEXT("hp"), Out.Hp) && JsonRead::ReadOptionalInt32Field(*Caps, TEXT("strength"), Out.Strength)
		&& JsonRead::ReadOptionalInt32Field(*Caps, TEXT("skill"), Out.Skill) && JsonRead::ReadOptionalInt32Field(*Caps, TEXT("speed"), Out.Speed)
		&& JsonRead::ReadOptionalInt32Field(*Caps, TEXT("defense"), Out.Defense);
}

// One promotion option: {"level", "to", "bonus"?}.
bool ReadPromotionOption(const TSharedPtr<FJsonObject>& Option, FClassPromotion& Out)
{
	if (!JsonRead::ReadInt32Field(Option, TEXT("level"), Out.MinLevel) || !JsonRead::ReadStringField(Option, TEXT("to"), Out.TargetClass))
	{
		return false;
	}
	if (!Option->HasField(TEXT("bonus")))
	{
		return true;
	}
	const TSharedPtr<FJsonObject>* Bonus = nullptr;
	commander::domain::StatBonus& Bonuses = Out.Bonus;
	return Option->TryGetObjectField(TEXT("bonus"), Bonus) && JsonRead::ReadOptionalInt32Field(*Bonus, TEXT("hp"), Bonuses.Hp)
		&& JsonRead::ReadOptionalInt32Field(*Bonus, TEXT("strength"), Bonuses.Strength)
		&& JsonRead::ReadOptionalInt32Field(*Bonus, TEXT("skill"), Bonuses.Skill) && JsonRead::ReadOptionalInt32Field(*Bonus, TEXT("speed"), Bonuses.Speed)
		&& JsonRead::ReadOptionalInt32Field(*Bonus, TEXT("defense"), Bonuses.Defense)
		&& JsonRead::ReadOptionalInt32Field(*Bonus, TEXT("movement"), Bonuses.Movement);
}

// "promotions": [ option, ... ]. At most MaxPromotionOptions, each to a different target class.
bool ReadPromotions(const TSharedPtr<FJsonObject>& ClassObject, FClassSpec& Spec)
{
	if (!ClassObject->HasField(TEXT("promotions")))
	{
		return true;
	}
	const TArray<TSharedPtr<FJsonValue>>* Options = nullptr;
	if (!ClassObject->TryGetArrayField(TEXT("promotions"), Options) || static_cast<size_t>(Options->Num()) > commander::domain::MaxPromotionOptions)
	{
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Entry : *Options)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		FClassPromotion Option;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object) || !ReadPromotionOption(*Object, Option))
		{
			return false;
		}
		for (const FClassPromotion& Existing : Spec.Promotions)
		{
			if (Existing.TargetClass == Option.TargetClass)
			{
				return false;
			}
		}
		Spec.Promotions.Add(Option);
	}
	return true;
}
} // namespace

commander::application::Result<FClassTable, EClassJsonError> FClassJsonParser::Parse(const FString& JsonText)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return ParseResult::Err(EClassJsonError::InvalidJson);
	}

	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Root->TryGetArrayField(TEXT("classes"), Entries))
	{
		return ParseResult::Err(EClassJsonError::InvalidField);
	}

	FClassTable Table;
	for (const TSharedPtr<FJsonValue>& Entry : *Entries)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		FString Id;
		FClassSpec Spec;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object) || !JsonRead::ReadStringField(*Object, TEXT("id"), Id)
			|| !JsonRead::ReadInt32Field(*Object, TEXT("movement"), Spec.Movement) || !ReadGrowth(*Object, Spec.Growth)
			|| !ReadCaps(*Object, Spec.Caps) || !ReadPromotions(*Object, Spec) || Table.Contains(Id))
		{
			return ParseResult::Err(EClassJsonError::InvalidField);
		}
		Table.Add(Id, Spec);
	}
	return ParseResult::Ok(std::move(Table));
}
