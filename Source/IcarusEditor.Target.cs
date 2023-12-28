// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class IcarusEditorTarget : TargetRules
{
	public IcarusEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

		ExtraModuleNames.AddRange(new[]
		{
			"Icarus", "WeaponSystem"
		});

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			ProjectDefinitions.Add("ICARUS_BUILD_TESTS");
			ExtraModuleNames.AddRange(new[]
			{
				"Tests"
			});
		}
	}
}