// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once 
#include "GameFramework/HUD.h"
#include "UE4_MinecraftHUD.generated.h"

UCLASS()
class AUE4_MinecraftHUD : public AHUD
{
	GENERATED_BODY()

public:
	AUE4_MinecraftHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

