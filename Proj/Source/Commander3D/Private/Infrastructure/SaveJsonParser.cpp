#include "Infrastructure/SaveJsonParser.h"

#include "Infrastructure/JsonReadHelpers.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
using namespace commander::domain;
using commander::application::Result;

constexpr int32 SaveVersion = 1;

void SetInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name, int32 Value)
{
	Object->SetNumberField(Name, static_cast<double>(Value));
}

bool GetObject(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Name, TSharedPtr<FJsonObject>& Out)
{
	const TSharedPtr<FJsonObject>* Found = nullptr;
	if (!Parent->TryGetObjectField(Name, Found) || !Found->IsValid())
	{
		return false;
	}
	Out = *Found;
	return true;
}

// ---- Growth / StatCaps / StatBonus (the same five keys) ----

template <typename T>
TSharedPtr<FJsonObject> FiveToJson(const T& V)
{
	TSharedPtr<FJsonObject> Object = MakeShared<FJsonObject>();
	SetInt(Object, TEXT("hp"), V.Hp);
	SetInt(Object, TEXT("strength"), V.Strength);
	SetInt(Object, TEXT("skill"), V.Skill);
	SetInt(Object, TEXT("speed"), V.Speed);
	SetInt(Object, TEXT("defense"), V.Defense);
	return Object;
}

template <typename T>
bool ReadFive(const TSharedPtr<FJsonObject>& Object, T& V)
{
	return JsonRead::ReadInt32Field(Object, TEXT("hp"), V.Hp) && JsonRead::ReadInt32Field(Object, TEXT("strength"), V.Strength)
		&& JsonRead::ReadInt32Field(Object, TEXT("skill"), V.Skill) && JsonRead::ReadInt32Field(Object, TEXT("speed"), V.Speed)
		&& JsonRead::ReadInt32Field(Object, TEXT("defense"), V.Defense);
}

template <typename T>
bool ReadFiveField(const TSharedPtr<FJsonObject>& Parent, const TCHAR* Name, T& V)
{
	TSharedPtr<FJsonObject> Object;
	return GetObject(Parent, Name, Object) && ReadFive(Object, V);
}

// ---- weapons and items ----

TSharedPtr<FJsonObject> WeaponToJson(const WeaponSpec& W)
{
	TSharedPtr<FJsonObject> Object = MakeShared<FJsonObject>();
	SetInt(Object, TEXT("might"), W.Might);
	SetInt(Object, TEXT("hit"), W.Hit);
	SetInt(Object, TEXT("minRange"), W.MinRange);
	SetInt(Object, TEXT("maxRange"), W.MaxRange);
	SetInt(Object, TEXT("durability"), W.Durability);
	SetInt(Object, TEXT("type"), W.Type);
	SetInt(Object, TEXT("requiredRank"), W.RequiredRank);
	SetInt(Object, TEXT("avoid"), W.Avoid);
	SetInt(Object, TEXT("rankHit"), W.RankHit);
	SetInt(Object, TEXT("rankAvoid"), W.RankAvoid);
	return Object;
}

bool ReadWeapon(const TSharedPtr<FJsonObject>& Object, WeaponSpec& W)
{
	return JsonRead::ReadInt32Field(Object, TEXT("might"), W.Might) && JsonRead::ReadInt32Field(Object, TEXT("hit"), W.Hit)
		&& JsonRead::ReadInt32Field(Object, TEXT("minRange"), W.MinRange) && JsonRead::ReadInt32Field(Object, TEXT("maxRange"), W.MaxRange)
		&& JsonRead::ReadInt32Field(Object, TEXT("durability"), W.Durability) && JsonRead::ReadInt32Field(Object, TEXT("type"), W.Type)
		&& JsonRead::ReadInt32Field(Object, TEXT("requiredRank"), W.RequiredRank) && JsonRead::ReadInt32Field(Object, TEXT("avoid"), W.Avoid)
		&& JsonRead::ReadInt32Field(Object, TEXT("rankHit"), W.RankHit) && JsonRead::ReadInt32Field(Object, TEXT("rankAvoid"), W.RankAvoid);
}

const TCHAR* KindName(ItemKind Kind)
{
	switch (Kind)
	{
	case ItemKind::Weapon: return TEXT("weapon");
	case ItemKind::Potion: return TEXT("potion");
	case ItemKind::RepairKit: return TEXT("repair");
	case ItemKind::Staff: return TEXT("staff");
	case ItemKind::PromotionSeal: return TEXT("seal");
	}
	return TEXT("");
}

bool KindFromName(const FString& Name, ItemKind& Out)
{
	for (const ItemKind Kind : {ItemKind::Weapon, ItemKind::Potion, ItemKind::RepairKit, ItemKind::Staff, ItemKind::PromotionSeal})
	{
		if (Name == KindName(Kind))
		{
			Out = Kind;
			return true;
		}
	}
	return false;
}

TSharedPtr<FJsonObject> ItemToJson(const ItemSlot& Item)
{
	TSharedPtr<FJsonObject> Object = MakeShared<FJsonObject>();
	Object->SetStringField(TEXT("kind"), KindName(Item.Kind));
	SetInt(Object, TEXT("uses"), Item.Uses);
	SetInt(Object, TEXT("amount"), Item.Amount);
	SetInt(Object, TEXT("range"), Item.Range);
	SetInt(Object, TEXT("price"), Item.Price);
	if (Item.Kind == ItemKind::Weapon)
	{
		Object->SetObjectField(TEXT("weapon"), WeaponToJson(Item.Weapon));
	}
	return Object;
}

bool ReadItem(const TSharedPtr<FJsonObject>& Object, ItemSlot& Item)
{
	FString Kind;
	if (!JsonRead::ReadStringField(Object, TEXT("kind"), Kind) || !KindFromName(Kind, Item.Kind)
		|| !JsonRead::ReadInt32Field(Object, TEXT("uses"), Item.Uses) || !JsonRead::ReadInt32Field(Object, TEXT("amount"), Item.Amount)
		|| !JsonRead::ReadInt32Field(Object, TEXT("range"), Item.Range) || !JsonRead::ReadInt32Field(Object, TEXT("price"), Item.Price))
	{
		return false;
	}
	if (Item.Kind == ItemKind::Weapon)
	{
		TSharedPtr<FJsonObject> Weapon;
		return GetObject(Object, TEXT("weapon"), Weapon) && ReadWeapon(Weapon, Item.Weapon);
	}
	return true;
}

// ---- units ----

bool TypeNameOf(const FBattleDataTables& Tables, int32 TypeId, FString& Out)
{
	for (const TPair<FString, int32>& Pair : Tables.Weapons.Types)
	{
		if (Pair.Value == TypeId)
		{
			Out = Pair.Key;
			return true;
		}
	}
	return false;
}

bool UnitToJson(const BattleUnit& Unit, const FBattleDataTables& Tables, TSharedPtr<FJsonObject>& Out)
{
	Out = MakeShared<FJsonObject>();
	SetInt(Out, TEXT("id"), static_cast<int32>(Unit.Id.Value));
	SetInt(Out, TEXT("hp"), Unit.MaxHp);
	SetInt(Out, TEXT("movement"), Unit.Movement);
	SetInt(Out, TEXT("strength"), Unit.Strength);
	SetInt(Out, TEXT("skill"), Unit.Skill);
	SetInt(Out, TEXT("speed"), Unit.Speed);
	SetInt(Out, TEXT("defense"), Unit.Defense);
	SetInt(Out, TEXT("level"), Unit.Level);
	SetInt(Out, TEXT("exp"), Unit.Experience);
	Out->SetBoolField(TEXT("boss"), Unit.IsBoss);

	TSharedPtr<FJsonObject> Weapon = WeaponToJson(Unit.Weapon);
	SetInt(Weapon, TEXT("uses"), Unit.WeaponUses);
	Out->SetObjectField(TEXT("weapon"), Weapon);
	Out->SetObjectField(TEXT("growth"), FiveToJson(Unit.Growth));
	Out->SetObjectField(TEXT("caps"), FiveToJson(Unit.Caps));

	TArray<TSharedPtr<FJsonValue>> Promotions;
	for (const PromotionSpec& Promotion : Unit.Promotions)
	{
		TSharedPtr<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("name"), UTF8_TO_TCHAR(Promotion.Name.c_str()));
		SetInt(Object, TEXT("level"), Promotion.MinLevel);
		TSharedPtr<FJsonObject> Bonus = FiveToJson(Promotion.Bonus);
		SetInt(Bonus, TEXT("movement"), Promotion.Bonus.Movement);
		Object->SetObjectField(TEXT("bonus"), Bonus);
		Object->SetObjectField(TEXT("growth"), FiveToJson(Promotion.Growth));
		Object->SetObjectField(TEXT("caps"), FiveToJson(Promotion.Caps));
		Promotions.Add(MakeShared<FJsonValueObject>(Object));
	}
	Out->SetArrayField(TEXT("promotions"), Promotions);

	TArray<TSharedPtr<FJsonValue>> Items;
	for (const ItemSlot& Item : Unit.Items)
	{
		Items.Add(MakeShared<FJsonValueObject>(ItemToJson(Item)));
	}
	Out->SetArrayField(TEXT("items"), Items);

	TSharedPtr<FJsonObject> Points = MakeShared<FJsonObject>();
	for (int32 TypeId = 1; TypeId <= MaxWeaponTypes; ++TypeId)
	{
		const int32 Value = Unit.WeaponPoints[static_cast<size_t>(TypeId)];
		if (Value == 0)
		{
			continue;
		}
		FString Name;
		if (!TypeNameOf(Tables, TypeId, Name))
		{
			return false;
		}
		SetInt(Points, *Name, Value);
	}
	Out->SetObjectField(TEXT("weaponPoints"), Points);
	return true;
}

ESaveJsonError UnitFromJson(const TSharedPtr<FJsonObject>& Object, const FBattleDataTables& Tables, BattleUnit& Unit)
{
	int64 Id = 0;
	TSharedPtr<FJsonObject> Weapon;
	const TArray<TSharedPtr<FJsonValue>>* Promotions = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
	if (!JsonRead::ReadIntFieldInRange(Object, TEXT("id"), 0, MAX_uint32, Id) || !JsonRead::ReadInt32Field(Object, TEXT("hp"), Unit.MaxHp)
		|| !JsonRead::ReadInt32Field(Object, TEXT("movement"), Unit.Movement) || !JsonRead::ReadInt32Field(Object, TEXT("strength"), Unit.Strength)
		|| !JsonRead::ReadInt32Field(Object, TEXT("skill"), Unit.Skill) || !JsonRead::ReadInt32Field(Object, TEXT("speed"), Unit.Speed)
		|| !JsonRead::ReadInt32Field(Object, TEXT("defense"), Unit.Defense) || !JsonRead::ReadInt32Field(Object, TEXT("level"), Unit.Level)
		|| !JsonRead::ReadInt32Field(Object, TEXT("exp"), Unit.Experience) || !JsonRead::ReadBoolField(Object, TEXT("boss"), Unit.IsBoss)
		|| !GetObject(Object, TEXT("weapon"), Weapon) || !ReadWeapon(Weapon, Unit.Weapon) || !JsonRead::ReadInt32Field(Weapon, TEXT("uses"), Unit.WeaponUses)
		|| !ReadFiveField(Object, TEXT("growth"), Unit.Growth) || !ReadFiveField(Object, TEXT("caps"), Unit.Caps)
		|| !Object->TryGetArrayField(TEXT("promotions"), Promotions) || !Object->TryGetArrayField(TEXT("items"), Items))
	{
		return ESaveJsonError::InvalidField;
	}
	Unit.Id = UnitId{static_cast<uint32>(Id)};
	Unit.Side = Faction::Player;
	Unit.Hp = Unit.MaxHp;

	for (const TSharedPtr<FJsonValue>& Entry : *Promotions)
	{
		const TSharedPtr<FJsonObject>* Found = nullptr;
		PromotionSpec Promotion;
		FString Name;
		TSharedPtr<FJsonObject> Bonus;
		if (!Entry.IsValid() || !Entry->TryGetObject(Found) || !JsonRead::ReadStringField(*Found, TEXT("name"), Name)
			|| !JsonRead::ReadInt32Field(*Found, TEXT("level"), Promotion.MinLevel) || !GetObject(*Found, TEXT("bonus"), Bonus)
			|| !ReadFive(Bonus, Promotion.Bonus) || !JsonRead::ReadInt32Field(Bonus, TEXT("movement"), Promotion.Bonus.Movement)
			|| !ReadFiveField(*Found, TEXT("growth"), Promotion.Growth) || !ReadFiveField(*Found, TEXT("caps"), Promotion.Caps))
		{
			return ESaveJsonError::InvalidField;
		}
		Promotion.Name = TCHAR_TO_UTF8(*Name);
		Unit.Promotions.push_back(std::move(Promotion));
	}

	for (const TSharedPtr<FJsonValue>& Entry : *Items)
	{
		const TSharedPtr<FJsonObject>* Found = nullptr;
		ItemSlot Item;
		if (!Entry.IsValid() || !Entry->TryGetObject(Found) || !ReadItem(*Found, Item))
		{
			return ESaveJsonError::InvalidField;
		}
		Unit.Items.push_back(Item);
	}

	if (Object->HasField(TEXT("weaponPoints")))
	{
		TSharedPtr<FJsonObject> Points;
		if (!GetObject(Object, TEXT("weaponPoints"), Points))
		{
			return ESaveJsonError::InvalidField;
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Points->Values)
		{
			int64 Value = 0;
			if (!JsonRead::ToInt(Pair.Value, 0, MAX_int32, Value))
			{
				return ESaveJsonError::InvalidField;
			}
			const int32* TypeId = Tables.Weapons.Types.Find(Pair.Key);
			if (TypeId == nullptr)
			{
				return ESaveJsonError::UnknownWeaponType;
			}
			Unit.WeaponPoints[static_cast<size_t>(*TypeId)] = static_cast<int32>(Value);
		}
	}
	return ESaveJsonError::None;
}
} // namespace

Result<FString, ESaveJsonError> FSaveJsonParser::Serialize(const FSaveData& Data, const FBattleDataTables& Tables)
{
	using Out = Result<FString, ESaveJsonError>;
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	SetInt(Root, TEXT("version"), SaveVersion);
	Root->SetStringField(TEXT("campaign"), Data.CampaignName);
	Root->SetStringField(TEXT("difficulty"), Data.Difficulty == Difficulty::Casual ? TEXT("casual") : TEXT("classic"));
	SetInt(Root, TEXT("chapter"), Data.ChapterIndex);
	SetInt(Root, TEXT("gold"), Data.Roster.Gold());

	TArray<TSharedPtr<FJsonValue>> Units;
	for (const BattleUnit& Unit : Data.Roster.Members())
	{
		TSharedPtr<FJsonObject> Object;
		if (!UnitToJson(Unit, Tables, Object))
		{
			return Out::Err(ESaveJsonError::UnknownWeaponType);
		}
		Units.Add(MakeShared<FJsonValueObject>(Object));
	}
	Root->SetArrayField(TEXT("units"), Units);

	FString Text;
	const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Text);
	if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
	{
		return Out::Err(ESaveJsonError::InvalidField);
	}
	return Out::Ok(MoveTemp(Text));
}

Result<FSaveData, ESaveJsonError> FSaveJsonParser::Parse(const FString& JsonText, const FBattleDataTables& Tables)
{
	using Out = Result<FSaveData, ESaveJsonError>;
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonText), Root) || !Root.IsValid())
	{
		return Out::Err(ESaveJsonError::InvalidJson);
	}
	int32 Version = 0;
	if (!JsonRead::ReadInt32Field(Root, TEXT("version"), Version) || Version != SaveVersion)
	{
		return Out::Err(ESaveJsonError::UnsupportedVersion);
	}

	FSaveData Data;
	FString DifficultyName;
	int32 Gold = 0;
	int64 Chapter = 0;
	const TArray<TSharedPtr<FJsonValue>>* Units = nullptr;
	if (!JsonRead::ReadStringField(Root, TEXT("campaign"), Data.CampaignName) || !JsonRead::ReadStringField(Root, TEXT("difficulty"), DifficultyName)
		|| !JsonRead::ReadIntFieldInRange(Root, TEXT("chapter"), 0, MAX_int32, Chapter) || !JsonRead::ReadInt32Field(Root, TEXT("gold"), Gold)
		|| !Root->TryGetArrayField(TEXT("units"), Units) || (DifficultyName != TEXT("casual") && DifficultyName != TEXT("classic")))
	{
		return Out::Err(ESaveJsonError::InvalidField);
	}
	Data.Difficulty = DifficultyName == TEXT("casual") ? Difficulty::Casual : Difficulty::Classic;
	Data.ChapterIndex = static_cast<int32>(Chapter);
	if (!Data.Roster.SetGold(Gold))
	{
		return Out::Err(ESaveJsonError::InvalidField);
	}

	for (const TSharedPtr<FJsonValue>& Entry : *Units)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		BattleUnit Unit;
		if (!Entry.IsValid() || !Entry->TryGetObject(Object))
		{
			return Out::Err(ESaveJsonError::InvalidField);
		}
		const ESaveJsonError Error = UnitFromJson(*Object, Tables, Unit);
		if (Error != ESaveJsonError::None)
		{
			return Out::Err(Error);
		}
		if (!Data.Roster.Add(Unit))
		{
			return Out::Err(ESaveJsonError::InvalidField);
		}
	}
	return Out::Ok(MoveTemp(Data));
}
