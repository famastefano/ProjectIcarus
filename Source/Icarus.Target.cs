// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class IcarusTarget : TargetRules
{
	public IcarusTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.AddRange(new string[] { "Icarus" });
		RegisterModulesCreatedByRider();

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
			ProjectDefinitions.Add("ICARUS_BUILD_TESTS");
	}

	private void RegisterModulesCreatedByRider()
	{
		ExtraModuleNames.AddRange(new string[]
		{
			"WeaponSystem",
			"Tests"
		});
	}
}