// Copyright Epic Games, Inc. All Rights Reserved.

#include "QuemadosCagadosGameMode.h"
#include "QuemadosCagadosCharacter.h"
#include "UObject/ConstructorHelpers.h"

AQuemadosCagadosGameMode::AQuemadosCagadosGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
