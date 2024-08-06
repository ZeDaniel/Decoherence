// Fill out your copyright notice in the Description page of Project Settings.


#include "DecoherenceCustomGravityClone.h"

//DON'T call super because we are overriding movement technology completely
void ADecoherenceCustomGravityClone::Move(const FInputActionValue& Value)
{
	AlterMoveInput(Value);

	//Gravity move code
	FVector2D MoveAxisVector = AlteredInput.Get<FVector2D>();


	AddMovementInput(LRWorldDirection, MoveAxisVector.X);
	AddMovementInput(FBWorldDirection, MoveAxisVector.Y);
}

void ADecoherenceCustomGravityClone::Look(const FInputActionValue& Value)
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

void ADecoherenceCustomGravityClone::Jump()
{
	Super::Jump();
}

void ADecoherenceCustomGravityClone::StopJumping()
{
	Super::StopJumping();
}