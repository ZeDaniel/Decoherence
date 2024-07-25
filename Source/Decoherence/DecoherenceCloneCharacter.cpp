// Fill out your copyright notice in the Description page of Project Settings.


#include "DecoherenceCloneCharacter.h"

void ADecoherenceCloneCharacter::Move(const FInputActionValue& Value)
{
	AlterMoveInput(Value);

	Super::Move(AlteredInput);
}

void ADecoherenceCloneCharacter::Look(const FInputActionValue& Value)
{
	AlterLookInput(Value);

	// input is a Vector2D
	FVector2D LookAxisVector = AlteredInput.Get<FVector2D>();

	if (Controller != nullptr)
	{
		FRotator NewRotation = GetActorRotation();
		//NewRotation.Pitch += LookAxisVector.Y;
		NewRotation.Yaw += LookAxisVector.X;

		SetActorRotation(NewRotation);
	}
}

void ADecoherenceCloneCharacter::Jump()
{
	Super::Jump();
}

void ADecoherenceCloneCharacter::StopJumping()
{
	Super::StopJumping();
}
