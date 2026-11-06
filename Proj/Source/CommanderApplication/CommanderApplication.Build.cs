using UnrealBuildTool;

// Use-case module (Commands, Queries, Result). Public/ headers must stay free of UE types
// (see docs/architecture/overview.md). Only Private/CommanderApplicationModule.cpp may use UE headers.
public class CommanderApplication : ModuleRules
{
	public CommanderApplication(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.Add("CommanderDomain");
		PrivateDependencyModuleNames.Add("Core");
	}
}
