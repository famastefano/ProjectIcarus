using UnrealBuildTool;

public class ObjectPoolingSystemTest : ModuleRules
{
    public ObjectPoolingSystemTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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
                "ObjectPoolingSystemPlugin",
                "TestWorld"
            }
        );
    }
}