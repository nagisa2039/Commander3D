// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Commander3D : ModuleRules
{
	public Commander3D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// Public only for what public headers expose (MapJsonParser.h exposes CommanderApplication's MapData).
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CommanderApplication" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject", "Engine", "InputCore", "EnhancedInput", "Json", "CommanderDomain"
		});
	}
}
