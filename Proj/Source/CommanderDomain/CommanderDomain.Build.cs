using UnrealBuildTool;

// Game rules module. Public/ headers must stay free of UE types (see docs/architecture/overview.md).
// Only Private/CommanderDomainModule.cpp may use UE headers (module registration).
public class CommanderDomain : ModuleRules
{
	public CommanderDomain(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PrivateDependencyModuleNames.Add("Core");
	}
}
