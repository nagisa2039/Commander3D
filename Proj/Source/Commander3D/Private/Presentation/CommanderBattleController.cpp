#include "Presentation/CommanderBattleController.h"

#include "Battle/EnemyAi.h"

namespace
{
using namespace commander;

bool ToUnitId(int32 Value, domain::UnitId& Out)
{
	if (Value < 0)
	{
		return false;
	}
	Out = domain::UnitId{static_cast<uint32>(Value)};
	return true;
}

FString Describe(const domain::UnitMoved& E)
{
	const domain::GridCoord& To = E.Path.back();
	return FString::Printf(TEXT("Unit %u moved to (%d, %d)"), E.Unit.Value, To.X, To.Y);
}

FString Describe(const domain::UnitWaited& E)
{
	return FString::Printf(TEXT("Unit %u waited"), E.Unit.Value);
}

FString Describe(const domain::AttackResolved& E)
{
	FString Text;
	for (const domain::Strike& Strike : E.Strikes)
	{
		Text += Strike.Hit ? FString::Printf(TEXT("Unit %u hit unit %u for %d (Hp %d). "), Strike.Attacker.Value, Strike.Defender.Value, Strike.Damage, Strike.DefenderHp)
			: FString::Printf(TEXT("Unit %u missed unit %u. "), Strike.Attacker.Value, Strike.Defender.Value);
	}
	for (const domain::UnitId Defeated : E.Defeated)
	{
		Text += FString::Printf(TEXT("Unit %u was defeated. "), Defeated.Value);
	}
	for (const domain::LevelUp& Up : E.LevelUps)
	{
		Text += FString::Printf(TEXT("Unit %u grew a level. "), Up.Unit.Value);
	}
	return Text.TrimEnd();
}

FString Describe(const domain::ChestOpened& E)
{
	return E.Item.has_value() ? FString::Printf(TEXT("Unit %u opened a chest: an item"), E.Unit.Value)
							  : FString::Printf(TEXT("Unit %u opened a chest: %d gold"), E.Unit.Value, E.Gold);
}

FString Describe(const domain::PhaseChanged& E)
{
	return FString::Printf(TEXT("Turn %d: %s phase"), E.Turn, E.Side == domain::Faction::Player ? TEXT("player") : TEXT("enemy"));
}

template <typename Event>
FString Describe(const Event&)
{
	return TEXT("Something happened"); // other actions (items, staffs, promotion, shop) are not described yet
}
} // namespace

bool UCommanderBattleController::BeginBattle(UCommanderCampaignSubsystem* Campaign)
{
	if (Campaign == nullptr || IsBattleRunning())
	{
		return false;
	}
	std::optional<application::PreparedBattle> Prepared = Campaign->StartBattle();
	if (!Prepared.has_value())
	{
		return false;
	}
	Session.reset(); // a finished battle may be replaced (retry / next chapter); the session refers to Rng, so it goes first
	Rng.reset();
	CampaignSubsystem = Campaign;
	Deployed = Prepared->Deployed;
	bFinished = false;
	LastStep = ECommanderCampaignStep::BattleOngoing;
	Rng = std::make_unique<application::StdRandomSource>(static_cast<uint32>(FMath::Rand()));
	Session = std::make_unique<application::BattleSession>(std::move(Prepared->Battle), *Rng);
	OnChanged.Broadcast();
	return true;
}

ECommanderBattleResult UCommanderBattleController::GetResult() const
{
	if (Session == nullptr)
	{
		return ECommanderBattleResult::Ongoing;
	}
	switch (Session->GetBattle().Outcome())
	{
	case domain::BattleOutcome::PlayerWon: return ECommanderBattleResult::Won;
	case domain::BattleOutcome::PlayerLost: return ECommanderBattleResult::Lost;
	case domain::BattleOutcome::Ongoing: break;
	}
	return ECommanderBattleResult::Ongoing;
}

FIntPoint UCommanderBattleController::GetMapSize() const
{
	if (Session == nullptr)
	{
		return FIntPoint::ZeroValue;
	}
	const domain::Grid& Grid = Session->GetBattle().GetMap().GetGrid();
	return FIntPoint(Grid.Width(), Grid.Height());
}

int32 UCommanderBattleController::GetCellMoveCost(int32 X, int32 Y) const
{
	if (Session == nullptr)
	{
		return -1;
	}
	const domain::TerrainSpec* Terrain = Session->GetBattle().GetMap().TerrainAt(domain::GridCoord{X, Y});
	return (Terrain != nullptr && Terrain->Passable) ? Terrain->MoveCost : -1;
}

int32 UCommanderBattleController::GetTurn() const
{
	return Session != nullptr ? Session->GetBattle().TurnNumber() : 0;
}

bool UCommanderBattleController::IsPlayerPhase() const
{
	return Session != nullptr && Session->GetBattle().CurrentFaction() == domain::Faction::Player;
}

int32 UCommanderBattleController::GetGold() const
{
	return Session != nullptr ? Session->GetBattle().Gold() : 0;
}

TArray<FCommanderBattleUnitView> UCommanderBattleController::GetUnits() const
{
	TArray<FCommanderBattleUnitView> Views;
	if (Session == nullptr)
	{
		return Views;
	}
	for (const domain::BattleUnit& Unit : Session->GetBattle().Units())
	{
		FCommanderBattleUnitView View;
		View.Id = static_cast<int32>(Unit.Id.Value);
		View.bPlayer = Unit.Side == domain::Faction::Player;
		View.bBoss = Unit.IsBoss;
		View.X = Unit.Position.X;
		View.Y = Unit.Position.Y;
		View.Hp = Unit.Hp;
		View.MaxHp = Unit.MaxHp;
		View.bHasMoved = Unit.HasMoved;
		View.bHasActed = Unit.HasActed;
		Views.Add(View);
	}
	return Views;
}

TArray<FCommanderBattleChestView> UCommanderBattleController::GetChests() const
{
	TArray<FCommanderBattleChestView> Views;
	if (Session != nullptr)
	{
		for (const domain::Chest& Chest : Session->GetBattle().Chests())
		{
			FCommanderBattleChestView View;
			View.X = Chest.Position.X;
			View.Y = Chest.Position.Y;
			View.bGold = !Chest.Item.has_value();
			Views.Add(View);
		}
	}
	return Views;
}

TArray<FIntPoint> UCommanderBattleController::GetShopCells() const
{
	TArray<FIntPoint> Cells;
	if (Session != nullptr)
	{
		for (const domain::Shop& Shop : Session->GetBattle().Shops())
		{
			Cells.Add(FIntPoint(Shop.Position.X, Shop.Position.Y));
		}
	}
	return Cells;
}

TArray<FIntPoint> UCommanderBattleController::GetMovableCells(int32 UnitId) const
{
	TArray<FIntPoint> Cells;
	domain::UnitId Id;
	if (Session == nullptr || !ToUnitId(UnitId, Id))
	{
		return Cells;
	}
	const domain::Battle& Battle = Session->GetBattle();
	const domain::BattleUnit* Unit = Battle.FindUnit(Id);
	if (Unit == nullptr || Unit->Side != Battle.CurrentFaction() || Unit->HasMoved || Unit->HasActed
		|| Battle.Outcome() != domain::BattleOutcome::Ongoing)
	{
		return Cells;
	}
	for (const domain::GridCoord& Cell : domain::CalculateMovementRange(Battle.GetMap(), Battle.Units(), *Unit).StoppableCells())
	{
		Cells.Add(FIntPoint(Cell.X, Cell.Y));
	}
	return Cells;
}

TArray<int32> UCommanderBattleController::GetAttackableTargets(int32 UnitId) const
{
	TArray<int32> Targets;
	domain::UnitId Id;
	if (Session == nullptr || !ToUnitId(UnitId, Id))
	{
		return Targets;
	}
	const domain::Battle& Battle = Session->GetBattle();
	const domain::BattleUnit* Unit = Battle.FindUnit(Id);
	if (Unit == nullptr || Unit->Side != Battle.CurrentFaction() || Unit->HasActed || !domain::HasUsableWeapon(*Unit)
		|| Battle.Outcome() != domain::BattleOutcome::Ongoing)
	{
		return Targets;
	}
	for (const domain::BattleUnit& Other : Battle.Units())
	{
		if (Other.Side != Unit->Side && domain::WeaponReaches(*Unit, domain::ManhattanDistance(Unit->Position, Other.Position)))
		{
			Targets.Add(static_cast<int32>(Other.Id.Value));
		}
	}
	return Targets;
}

bool UCommanderBattleController::MoveUnit(int32 UnitId, int32 X, int32 Y)
{
	domain::UnitId Id;
	return ToUnitId(UnitId, Id) && Run(application::MoveUnitCommand{Id, domain::GridCoord{X, Y}});
}

bool UCommanderBattleController::WaitUnit(int32 UnitId)
{
	domain::UnitId Id;
	return ToUnitId(UnitId, Id) && Run(application::WaitUnitCommand{Id});
}

bool UCommanderBattleController::AttackUnit(int32 AttackerId, int32 TargetId)
{
	domain::UnitId Attacker;
	domain::UnitId Target;
	return ToUnitId(AttackerId, Attacker) && ToUnitId(TargetId, Target) && Run(application::AttackUnitCommand{Attacker, Target});
}

bool UCommanderBattleController::OpenChest(int32 UnitId)
{
	domain::UnitId Id;
	return ToUnitId(UnitId, Id) && Run(application::OpenChestCommand{Id});
}

bool UCommanderBattleController::EndPlayerPhase()
{
	if (!IsBattleRunning() || !IsPlayerPhase() || !Run(application::EndPhaseCommand{}))
	{
		return false;
	}
	if (IsBattleRunning() && !IsPlayerPhase())
	{
		for (const application::BattleEvent& Event : application::RunEnemyPhase(*Session))
		{
			Announce(Event);
		}
		OnChanged.Broadcast();
		ReportIfFinished();
	}
	return true;
}

bool UCommanderBattleController::Run(const application::BattleCommand& Command)
{
	if (!IsBattleRunning() || !IsPlayerPhase())
	{
		return false;
	}
	const application::CommandResult Result = Session->Execute(Command);
	if (!Result.IsOk())
	{
		return false;
	}
	Announce(Result.Value());
	OnChanged.Broadcast();
	ReportIfFinished();
	return true;
}

void UCommanderBattleController::Announce(const application::BattleEvent& Event)
{
	OnLog.Broadcast(std::visit([](const auto& E) { return Describe(E); }, Event));
}

void UCommanderBattleController::ReportIfFinished()
{
	if (bFinished || Session == nullptr || Session->GetBattle().Outcome() == domain::BattleOutcome::Ongoing)
	{
		return;
	}
	bFinished = true;
	if (CampaignSubsystem != nullptr)
	{
		LastStep = CampaignSubsystem->CompleteBattle(Session->GetBattle(), Deployed);
	}
	OnFinished.Broadcast(GetResult(), LastStep);
}
