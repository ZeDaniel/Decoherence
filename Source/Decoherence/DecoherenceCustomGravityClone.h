// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DecoherenceCloneCharacter.h"
#include "DecoherenceCustomGravityClone.generated.h"

/**
 * 
 */
UCLASS()
class DECOHERENCE_API ADecoherenceCustomGravityClone : public ADecoherenceCloneCharacter
{
	GENERATED_BODY()

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value) override;

	/** Called for looking input */
	virtual void Look(const FInputActionValue& Value) override;

	/** Called for jump input*/
	virtual void Jump() override;
	/** Called for stop jumping input*/
	virtual void StopJumping() override;

	//World directions for left right movement and forward backward movement
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Gravity")
	FVector LRWorldDirection = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Gravity")
	FVector FBWorldDirection = FVector::ZeroVector;
	
};
