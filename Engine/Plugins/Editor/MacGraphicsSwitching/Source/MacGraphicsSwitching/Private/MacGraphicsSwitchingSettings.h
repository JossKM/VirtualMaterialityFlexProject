// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MacGraphicsSwitchingSettings.generated.h"

UCLASS(Config=EditorSettings)
class UMacGraphicsSwitchingSettings : public UObject
{
	GENERATED_UCLASS_BODY()
	
public:
	
	UPROPERTY(Config, EditAnywhere, Category=RHI, meta = (ConfigRestartRequired=true))
	int32 RendererID;
};
