// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TestWorld : ModuleRules
{
	public TestWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);
        
		if(Target.Type == TargetType.Editor)
			PrivateDependencyModuleNames.Add("UnrealEd");    

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
			}
		);
	}
}
