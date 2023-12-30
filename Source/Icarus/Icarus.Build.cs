// Stefano Famà (famastefano@gmail.com)

using UnrealBuildTool;

public class Icarus : ModuleRules
{
	public Icarus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"WeaponSystemPlugin"
		});
	}
}