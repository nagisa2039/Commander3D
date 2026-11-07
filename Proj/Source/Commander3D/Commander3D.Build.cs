// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Commander3D : ModuleRules
{
	public Commander3D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// Public only for what public headers expose (MapJsonParser.h exposes CommanderApplication's MapData; BattleJsonParser.h exposes Json's FJsonObject; the Presentation subsystem exposes CoreUObject / Engine types).
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "Json", "CommanderApplication" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore", "EnhancedInput", "CommanderDomain"
		});
	}
}
