using UnrealBuildTool;

// Editor-only UE Automation tests for the UE-facing layers (module wiring, Infrastructure, Adapters).
// Domain / Application rules are tested without Unreal in Tests/Core.
public class CommanderTests : ModuleRules
{
	public CommanderTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "Json", "CommanderDomain", "CommanderApplication", "Commander3D" });
	}
}
