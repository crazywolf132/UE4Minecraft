// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UE4_Minecraft.h"
#include "UE4_MinecraftGameMode.h"
#include "UE4_MinecraftHUD.h"
#include "UE4_MinecraftCharacter.h"

AUE4_MinecraftGameMode::AUE4_MinecraftGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AUE4_MinecraftHUD::StaticClass();
}
