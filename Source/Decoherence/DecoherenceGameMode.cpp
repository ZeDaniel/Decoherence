// Copyright Epic Games, Inc. All Rights Reserved.

#include "DecoherenceGameMode.h"
#include "DecoherenceCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADecoherenceGameMode::ADecoherenceGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
