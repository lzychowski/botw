// Copyright Epic Games, Inc. All Rights Reserved.

#include "BotwGameMode.h"
#include "BotwCharacter.h"
#include "UObject/ConstructorHelpers.h"

ABotwGameMode::ABotwGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
