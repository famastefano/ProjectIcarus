using System;
using UnrealBuildTool;

public class Tests : ModuleRules
{
    public Tests(ReadOnlyTargetRules Target) : base(Target)
    {
        if(Target.Configuration == UnrealTargetConfiguration.Shipping)
            Console.Error.WriteLine("Test module is being built in Shipping");
        
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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
                "AutomationTest",
                
                // Tested modules
                "WeaponSystem",
            }
        );
    }
}