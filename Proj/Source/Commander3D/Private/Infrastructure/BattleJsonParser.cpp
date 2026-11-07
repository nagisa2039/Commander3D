#include "Infrastructure/BattleJsonParser.h"

#include "Battle/Progression.h"
#include "Infrastructure/JsonReadHelpers.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
using commander::application::Result;
using commander::domain::ItemKind;
using commander::domain::ItemSlot;
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

// Turns an id into an inventory/stock slot: a weapon id is a fresh spare weapon, any other id must be an item.
// Prices come from the tables.
bool ResolveItem(const FBattleDataTables& Tables, const FString& Id, ItemSlot& Out)
{
	if (const commander::domain::WeaponSpec* Weapon = Tables.Weapons.Weapons.Find(Id))
	{
		Out = ItemSlot{};
		Out.Kind = ItemKind::Weapon;
		Out.Weapon = *Weapon;
		Out.Uses = Weapon->Durability;
		Out.Price = Tables.Weapons.Prices.FindRef(Id);
		return true;
	}
	if (const ItemSlot* Item = Tables.Items.Find(Id))
	{
		Out = *Item;
		return true;
	}
	return false;
}

// "weaponRanks": {"sword": 2, ...} -> training points (the lowest count that gives that rank).
EBattleJsonError ReadWeaponRanks(const TSharedPtr<FJsonObject>& Object, const FBattleDataTables& Tables, commander::domain::BattleUnit& Unit)
{
	if (!Object->HasField(TEXT("weaponRanks")))
	{
		return EBattleJsonError::None;
	}
	const TSharedPtr<FJsonObject>* Ranks = nullptr;
	if (!Object->TryGetObjectField(TEXT("weaponRanks"), Ranks))
	{
		return EBattleJsonError::InvalidField;
	}
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Ranks)->Values)
	{
		int64 Rank = 0;
		if (!JsonRead::ToInt(Pair.Value, 0, commander::domain::MaxWeaponRank, Rank))
		{
			return EBattleJsonError::InvalidField;
		}
		const int32* TypeId = Tables.Weapons.Types.Find(Pair.Key);
		if (TypeId == nullptr)
		{
			return EBattleJsonError::UnknownWeaponType;
		}
		Unit.WeaponPoints[static_cast<size_t>(*TypeId)] = commander::domain::RankThreshold[Rank];
	}
	return EBattleJsonError::None;
}

// Applies "class": movement, growth, caps and the promotion options (each target class gives the new growth / caps).
EBattleJsonError ApplyClass(const FBattleDataTables& Tables, const FString& ClassId, commander::domain::BattleUnit& Unit)
{
	const FClassSpec* Class = Tables.Classes.Find(ClassId);
	if (Class == nullptr)
	{
		return EBattleJsonError::UnknownClass;
	}
	Unit.Movement = Class->Movement;
	Unit.Growth = Class->Growth;
	Unit.Caps = Class->Caps;
	for (const FClassPromotion& Option : Class->Promotions)
	{
		const FClassSpec* Target = Tables.Classes.Find(Option.TargetClass);
		if (Target == nullptr)
		{
			return EBattleJsonError::UnknownClass;
		}
		commander::domain::PromotionSpec Promotion;
		Promotion.Name = TCHAR_TO_UTF8(*Option.TargetClass);
		Promotion.MinLevel = Option.MinLevel;
		Promotion.Bonus = Option.Bonus;
		Promotion.Growth = Target->Growth;
		Promotion.Caps = Target->Caps;
		Unit.Promotions.push_back(Promotion);
	}
	return EBattleJsonError::None;
}

// Fills Unit from a JSON object. Returns None on success.
EBattleJsonError ReadUnit(const TSharedPtr<FJsonObject>& Object, const FBattleDataTables& Tables, commander::domain::BattleUnit& Unit)
{
	using commander::domain::Faction;

	int64 Id = 0;
	FString Side;
	FString ClassId;
	TArray<FString> ItemIds;
	if (!JsonRead::ReadIntFieldInRange(Object, TEXT("id"), 0, MAX_uint32, Id) || !JsonRead::ReadStringField(Object, TEXT("side"), Side)
		|| !JsonRead::ReadInt32Field(Object, TEXT("x"), Unit.Position.X) || !JsonRead::ReadInt32Field(Object, TEXT("y"), Unit.Position.Y)
		|| !JsonRead::ReadInt32Field(Object, TEXT("hp"), Unit.Hp)
		|| !JsonRead::ReadInt32Field(Object, TEXT("strength"), Unit.Strength) || !JsonRead::ReadInt32Field(Object, TEXT("skill"), Unit.Skill)
		|| !JsonRead::ReadInt32Field(Object, TEXT("speed"), Unit.Speed) || !JsonRead::ReadInt32Field(Object, TEXT("defense"), Unit.Defense)
		|| !JsonRead::ReadOptionalBoolField(Object, TEXT("boss"), Unit.IsBoss)
		|| !JsonRead::ReadOptionalInt32Field(Object, TEXT("level"), Unit.Level) || !JsonRead::ReadOptionalInt32Field(Object, TEXT("exp"), Unit.Experience)
		|| !JsonRead::ReadOptionalStringField(Object, TEXT("class"), ClassId) || !JsonRead::ReadOptionalStringArrayField(Object, TEXT("items"), ItemIds))
	{
		return EBattleJsonError::InvalidField;
	}
	Unit.Id = commander::domain::UnitId{static_cast<uint32>(Id)};
	Unit.MaxHp = Unit.Hp;

	// The class supplies movement, growth, caps and promotion; an explicit "movement" overrides the movement,
	// and is required without a class.
	const bool bHasClass = Object->HasField(TEXT("class"));
	if (bHasClass)
	{
		const EBattleJsonError ClassError = ApplyClass(Tables, ClassId, Unit);
		if (ClassError != EBattleJsonError::None)
		{
			return ClassError;
		}
	}
	if (Object->HasField(TEXT("movement")) ? !JsonRead::ReadInt32Field(Object, TEXT("movement"), Unit.Movement) : !bHasClass)
	{
		return EBattleJsonError::InvalidField;
	}

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
		const commander::domain::WeaponSpec* Weapon = Tables.Weapons.Weapons.Find(WeaponId);
		if (Weapon == nullptr)
		{
			return EBattleJsonError::UnknownWeapon;
		}
		Unit.Weapon = *Weapon;
		Unit.WeaponUses = Weapon->Durability; // a fresh weapon
	}

	const EBattleJsonError RankError = ReadWeaponRanks(Object, Tables, Unit);
	if (RankError != EBattleJsonError::None)
	{
		return RankError;
	}

	for (const FString& ItemId : ItemIds)
	{
		ItemSlot Slot;
		if (!ResolveItem(Tables, ItemId, Slot))
		{
			return EBattleJsonError::UnknownItem;
		}
		Unit.Items.push_back(Slot);
	}
	return EBattleJsonError::None;
}

EBattleJsonError ReadShops(const TSharedPtr<FJsonObject>& Root, const FBattleDataTables& Tables, std::vector<commander::domain::Shop>& Out)
{
	if (!Root->HasField(TEXT("shops")))
	{
		return EBattleJsonError::None;
	}
	const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
	if (!Root->TryGetArrayField(TEXT("shops"), Entries))
	{
		return EBattleJsonError::InvalidField;
	}
	for (const TSharedPtr<FJsonValue>& Entry : *Entries)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		commander::domain::Shop Shop;
		TArray<FString> StockIds;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object) || !JsonRead::ReadInt32Field(*Object, TEXT("x"), Shop.Position.X)
			|| !JsonRead::ReadInt32Field(*Object, TEXT("y"), Shop.Position.Y) || !(*Object)->HasField(TEXT("stock"))
			|| !JsonRead::ReadOptionalStringArrayField(*Object, TEXT("stock"), StockIds))
		{
			return EBattleJsonError::InvalidField;
		}
		for (const FString& StockId : StockIds)
		{
			ItemSlot Slot;
			if (!ResolveItem(Tables, StockId, Slot))
			{
				return EBattleJsonError::UnknownItem;
			}
			Shop.Stock.push_back(Slot);
		}
		Out.push_back(std::move(Shop));
	}
	return EBattleJsonError::None;
}
} // namespace

commander::application::Result<FBattleDefinition, EBattleJsonError> FBattleJsonParser::Parse(const FString& JsonText, const FBattleDataTables& Tables)
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
		|| !ReadConditions(Root, Definition.Setup.Conditions) || !JsonRead::ReadOptionalInt32Field(Root, TEXT("gold"), Definition.Setup.Gold))
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
		const EBattleJsonError Error = ReadUnit(*Object, Tables, Unit);
		if (Error != EBattleJsonError::None)
		{
			return ParseResult::Err(Error);
		}
		Definition.Setup.Units.push_back(Unit);
	}

	const EBattleJsonError ShopError = ReadShops(Root, Tables, Definition.Setup.Shops);
	if (ShopError != EBattleJsonError::None)
	{
		return ParseResult::Err(ShopError);
	}
	return ParseResult::Ok(std::move(Definition));
}
