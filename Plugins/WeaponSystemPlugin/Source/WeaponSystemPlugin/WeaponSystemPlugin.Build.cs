// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WeaponSystemPlugin : ModuleRules
{
	public WeaponSystemPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"GameplayAbilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"CoreUObject",
				"Engine"
			}
		);

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
			PrivateDependencyModuleNames.AddRange(
				new[]
				{
					"TraceLog"
				});
	}
}
