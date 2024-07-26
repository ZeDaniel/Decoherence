// Copyright Epic Games, Inc. All Rights Reserved.

#include "DecoherenceGameMode.h"
#include "DecoherenceCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "UMG.h"

ADecoherenceGameMode::ADecoherenceGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

void ADecoherenceGameMode::PauseGame()
{
	bGamePaused = true;
	UGameplayStatics::SetGamePaused(GetWorld(), true);
	
	//Create pause widget if its template is present
	if (PauseWidgetClass)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			PauseWidget = CreateWidget<UUserWidget>(PlayerController, PauseWidgetClass, TEXT("Pause Widget"));
			PauseWidget->AddToViewport(9999);

			FInputModeGameAndUI PauseInputMode;
			PauseInputMode.SetWidgetToFocus(PauseWidget->GetSlateWidgetFromName(PauseWidget->GetFName()));

			PlayerController->SetInputMode(PauseInputMode);

			//UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetShowMouseCursor(true);
		}	
	}
}

void ADecoherenceGameMode::UnPauseGame()
{
	bGamePaused = false;
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	//Remove pause widget if present
	if (PauseWidget)
	{
		PauseWidget->RemoveFromParent();

		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			FInputModeGameOnly GameInputMode;
			
			PlayerController->SetInputMode(GameInputMode);
		}
		
		//UGameplayStatics::GetPlayerController(GetWorld(), 0)->SetShowMouseCursor(false);
	}
}

void ADecoherenceGameMode::RestartLevel()
{
	if (bGamePaused)
	{
		UnPauseGame();
	}

	UGameplayStatics::OpenLevel(GetWorld(), FName(UGameplayStatics::GetCurrentLevelName(GetWorld())));
}
