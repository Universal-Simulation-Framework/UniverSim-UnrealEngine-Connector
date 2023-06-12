// Copyright (c) 2022, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

using System.IO;
using UnrealBuildTool;

public class USConnectorEditor : ModuleRules
{
  public USConnectorEditor(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

    PublicDependencyModuleNames.AddRange(
      new string[]
      { "Core",
        "CoreUObject",
        "Engine",
        "BlueprintGraph",
        "AnimGraph",
        "AnimGraphRuntime",
        "USConnector"
      }
      );

    PrivateDependencyModuleNames.AddRange(new string[] { });

    // Uncomment if you are using Slate UI
    // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

    // Uncomment if you are using online features
    // PrivateDependencyModuleNames.Add("OnlineSubsystem");

    // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
  }
}
