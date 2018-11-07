// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
    public class BlastMeshEditor : ModuleRules
    {
        public BlastMeshEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PublicDependencyModuleNames.AddRange(
                new string[] {
                "Engine",
                "PhysX"
                }
            );

            PrivateDependencyModuleNames.AddRange(
                new string[] {
                "Blast",
                "BlastEditor",
                "Core",
                "CoreUObject",
                "InputCore",
                "RenderCore",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "UnrealEd",
                "Projects",
                "DesktopPlatform",
                "AdvancedPreviewScene",
                "AssetTools",
                "LevelEditor",
                "MeshMergeUtilities",
                "RawMesh",
                "MeshUtilities",
                "MeshUtilitiesCommon",
                "RHI",
                }
            );

            DynamicallyLoadedModuleNames.Add("PropertyEditor");

            string[] BlastLibs =
            {
                 "NvBlastExtAuthoring",
            };

            PrivateIncludePaths.AddRange(
                new string[]
                {
                    "Blast/Public/extensions/assetutils/include",
                    "Blast/Public/extensions/authoring/include",
                    "Blast/Public/extensions/serialization/include",
                    "Blast/Public/extensions/shaders/include",
                    "Blast/Public/extensions/stress/include",
                    "Blast/Public/globals/include",
                    "Blast/Public/lowlevel/include"
                }
            );

            Blast.SetupModuleBlastSupport(this, BlastLibs);

            AddEngineThirdPartyPrivateStaticDependencies(Target, "FBX");

            //SetupModulePhysXAPEXSupport(Target);
        }
    }
}