using UnrealBuildTool;

public class FaceTouchCorrectionEditor : ModuleRules
{
    public FaceTouchCorrectionEditor(ReadOnlyTargetRules target) : base(target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "FaceTouchCorrection",
                "Slate",
                "SlateCore"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "CoreUObject",
                "EditorFramework",
                "Engine",
                "InputCore",
                "UnrealEd"
            });
    }
}
