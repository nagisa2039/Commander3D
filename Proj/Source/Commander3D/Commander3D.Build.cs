// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Commander3D : ModuleRules
{
	public Commander3D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// No public headers expose these yet, so everything is Private. Move a module to Public
		// only when a public header of this module needs its types.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"CommanderDomain", "CommanderApplication"
		});
	}
}
