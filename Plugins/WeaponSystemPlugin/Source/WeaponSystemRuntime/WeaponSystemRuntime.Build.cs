using UnrealBuildTool;

public class WeaponSystemRuntime : ModuleRules
{
    public WeaponSystemRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core"
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