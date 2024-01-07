// Stefano Famà (famastefano@gmail.com)

using UnrealBuildTool;

public class Icarus : ModuleRules
{
	public Icarus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new []
		{
			"Core",
			"CoreUObject",
			"Engine",
		});
		
		PrivateDependencyModuleNames.AddRange(new []{
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"WeaponSystemPlugin",
			"ObjectPoolingSystemPlugin"
		});
	}
}