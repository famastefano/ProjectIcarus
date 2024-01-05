using UnrealBuildTool;

public class WeaponSystemTest : ModuleRules
{
    public WeaponSystemTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "AutomationTest",
                
                "TestWorld",
                
                "WeaponSystemPlugin",
            }
        );
    }
}