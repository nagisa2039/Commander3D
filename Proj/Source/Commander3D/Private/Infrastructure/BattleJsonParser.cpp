#include "Infrastructure/BattleJsonParser.h"

#include "Infrastructure/JsonReadHelpers.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
using commander::application::Result;
using ParseResult = Result<FBattleDefinition, EBattleJsonError>;

bool ReadConditions(const TSharedPtr<FJsonObject>& Root, commander::domain::VictoryConditions& Out)
{
	const TSharedPtr<FJsonValue> Field = Root->TryGetField(TEXT("conditions"));
	if (!Field.IsValid())
	{
		return true;
	}
	const TSharedPtr<FJsonObject>* Object = nullptr;
	if (!Field->TryGetObject(Object))
	{
		return false;
	}
	int64 TurnLimit = Out.TurnLimit;
	if ((*Object)->HasField(TEXT("turnLimit")) && !JsonRead::ReadIntFieldInRange(*Object, TEXT("turnLimit"), 0, MAX_int32, TurnLimit))
	{
		return false;
	}
	Out.TurnLimit = static_cast<int32>(TurnLimit);
	return JsonRead::ReadOptionalBoolField(*Object, TEXT("routEnemies"), Out.RoutEnemies)
		&& JsonRead::ReadOptionalBoolField(*Object, TEXT("defeatBoss"), Out.DefeatBoss);
}

// Fills Unit from a JSON object. Returns None on success.
EBattleJsonError ReadUnit(const TSharedPtr<FJsonObject>& Object, const FWeaponTable& Weapons, commander::domain::BattleUnit& Unit)
{
	using commander::domain::Faction;

	int64 Id = 0;
	FString Side;
	if (!JsonRead::ReadIntFieldInRange(Object, TEXT("id"), 0, MAX_uint32, Id) || !JsonRead::ReadStringField(Object, TEXT("side"), Side)
		|| !JsonRead::ReadInt32Field(Object, TEXT("x"), Unit.Position.X) || !JsonRead::ReadInt32Field(Object, TEXT("y"), Unit.Position.Y)
		|| !JsonRead::ReadInt32Field(Object, TEXT("movement"), Unit.Movement) || !JsonRead::ReadInt32Field(Object, TEXT("hp"), Unit.Hp)
		|| !JsonRead::ReadInt32Field(Object, TEXT("strength"), Unit.Strength) || !JsonRead::ReadInt32Field(Object, TEXT("skill"), Unit.Skill)
		|| !JsonRead::ReadInt32Field(Object, TEXT("speed"), Unit.Speed) || !JsonRead::ReadInt32Field(Object, TEXT("defense"), Unit.Defense)
		|| !JsonRead::ReadOptionalBoolField(Object, TEXT("boss"), Unit.IsBoss))
	{
		return EBattleJsonError::InvalidField;
	}
	Unit.Id = commander::domain::UnitId{static_cast<uint32>(Id)};
	Unit.MaxHp = Unit.Hp;

	if (Side == TEXT("player"))
	{
		Unit.Side = Faction::Player;
	}
	else if (Side == TEXT("enemy"))
	{
		Unit.Side = Faction::Enemy;
	}
	else
	{
		return EBattleJsonError::InvalidField;
	}

	if (Object->HasField(TEXT("weapon")))
	{
		FString WeaponId;
		if (!JsonRead::ReadStringField(Object, TEXT("weapon"), WeaponId))
		{
			return EBattleJsonError::InvalidField;
		}
		const commander::domain::WeaponSpec* Weapon = Weapons.Find(WeaponId);
		if (Weapon == nullptr)
		{
			return EBattleJsonError::UnknownWeapon;
		}
		Unit.Weapon = *Weapon;
	}
	return EBattleJsonError::None;
}
} // namespace

commander::application::Result<FBattleDefinition, EBattleJsonError> FBattleJsonParser::Parse(const FString& JsonText, const FWeaponTable& Weapons)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return ParseResult::Err(EBattleJsonError::InvalidJson);
	}

	FBattleDefinition Definition;
	const TArray<TSharedPtr<FJsonValue>>* Units = nullptr;
	if (!JsonRead::ReadStringField(Root, TEXT("map"), Definition.MapName) || !Root->TryGetArrayField(TEXT("units"), Units)
		|| !ReadConditions(Root, Definition.Setup.Conditions))
	{
		return ParseResult::Err(EBattleJsonError::InvalidField);
	}

	for (const TSharedPtr<FJsonValue>& Entry : *Units)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object))
		{
			return ParseResult::Err(EBattleJsonError::InvalidField);
		}
		commander::domain::BattleUnit Unit;
		const EBattleJsonError Error = ReadUnit(*Object, Weapons, Unit);
		if (Error != EBattleJsonError::None)
		{
			return ParseResult::Err(Error);
		}
		Definition.Setup.Units.push_back(Unit);
	}
	return ParseResult::Ok(std::move(Definition));
}
