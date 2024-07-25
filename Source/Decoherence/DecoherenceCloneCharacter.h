// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DecoherenceCharacter.h"
#include "InputActionValue.h"
#include "DecoherenceCloneCharacter.generated.h"

/**
 * 
 */
UCLASS()
class DECOHERENCE_API ADecoherenceCloneCharacter : public ADecoherenceCharacter
{
	GENERATED_BODY()

protected:

	UFUNCTION(BlueprintImplementableEvent)
	void AlterMoveInput(const FInputActionValue& Value);
	UFUNCTION(BlueprintImplementableEvent)
	void AlterLookInput(const FInputActionValue& Value);

	/** Called for movement input */
	void Move(const FInputActionValue& Value) override;

	/** Called for looking input */
	virtual void Look(const FInputActionValue& Value) override;

	/** Called for jump input*/
	virtual void Jump() override;
	/** Called for stop jumping input*/
	virtual void StopJumping() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FInputActionValue AlteredInput;
};
