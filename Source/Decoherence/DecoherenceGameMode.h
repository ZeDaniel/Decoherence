// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DecoherenceGameMode.generated.h"

UCLASS(minimalapi)
class ADecoherenceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADecoherenceGameMode();

	UFUNCTION(BlueprintCallable)
	void PauseGame();
	UFUNCTION(BlueprintCallable)
	void UnPauseGame();
	UFUNCTION(BlueprintCallable)
	void RestartLevel();

	bool GamePaused() { return bGamePaused; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Pause")
	TSubclassOf<UUserWidget> PauseWidgetClass;

private:

	bool bGamePaused = false;
	UUserWidget* PauseWidget;
};



