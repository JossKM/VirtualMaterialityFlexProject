// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderDerivedDataVersion.h: Shader derived data version.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"

// In case of merge conflicts with DDC versions, you MUST generate a new GUID and set this new
// guid as version

// NVCHANGE_BEGIN: Add VXGI
#if WITH_GFSDK_VXGI

//Our FShaderResource class is not compatible with this define on
#define GLOBALSHADERMAP_DERIVEDDATA_VER			TEXT("D3F7BDB378724EF8A0A949AA9F602899")
#define MATERIALSHADERMAP_DERIVEDDATA_VER		TEXT("7033AC2BA6164E209D50AC51D7D8EF30")

#else
// NVCHANGE_END: Add VXGI

#define GLOBALSHADERMAP_DERIVEDDATA_VER			TEXT("4A393A7FDF3048EF8376E103162A8ED6")
#define MATERIALSHADERMAP_DERIVEDDATA_VER		TEXT("F1CC9C7A48F5422EB67E3ADC82465189")

// NVCHANGE_BEGIN: Add VXGI
#endif
// NVCHANGE_END: Add VXGI