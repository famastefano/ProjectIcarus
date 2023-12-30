// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class IcarusEditorTarget : TargetRules
{
	public IcarusEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		CppStandard = CppStandardVersion.EngineDefault;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.AddRange(new[]
		{
			"Icarus"
		});
	}
}