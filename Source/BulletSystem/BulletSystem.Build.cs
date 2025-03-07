// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BulletSystem : ModuleRules
{
	public BulletSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PublicDependencyModuleNames.AddRange(new string[] { "BS_Weapon", "T_Hotbar" });
	}
}
