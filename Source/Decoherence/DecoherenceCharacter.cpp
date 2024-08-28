// Copyright Epic Games, Inc. All Rights Reserved.

#include "DecoherenceCharacter.h"
#include "DecoherenceProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "DecoherenceGameMode.h"

DEFINE_LOG_CATEGORY(LogDecoherenceCharacter);

//////////////////////////////////////////////////////////////////////////
// ADecoherenceCharacter

ADecoherenceCharacter::ADecoherenceCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

}

void ADecoherenceCharacter::AddClone(ADecoherenceCharacter* NewClone)
{
	if (NewClone)
	{
		PlayerClones.Add(NewClone);
	}
}

void ADecoherenceCharacter::RemoveClone(ADecoherenceCharacter* CloneToRemove)
{
	if (CloneToRemove)
	{
		PlayerClones.Remove(CloneToRemove);
	}
}

void ADecoherenceCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

//////////////////////////////////////////////////////////////////////////// Input

void ADecoherenceCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADecoherenceCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADecoherenceCharacter::Look);

		// Pausing
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Triggered, this, &ADecoherenceCharacter::Pause);
	}
	else
	{
		UE_LOG(LogDecoherenceCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ADecoherenceCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}

	//for all clones, duplicate input
	if (PlayerClones.Num() > 0)
	{
		for (ADecoherenceCharacter* Clone : PlayerClones)
		{
			Clone->Move(Value);
		}
	}
}

void ADecoherenceCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}

	//for all clones, duplicate input
	if (PlayerClones.Num() > 0)
	{
		for (ADecoherenceCharacter* Clone : PlayerClones)
		{
			Clone->Look(Value);
		}
	}
}

void ADecoherenceCharacter::Jump()
{
	Super::Jump();

	//for all clones, duplicate input
	if (PlayerClones.Num() > 0)
	{
		for (ADecoherenceCharacter* Clone : PlayerClones)
		{
			Clone->Jump();
		}
	}
}

void ADecoherenceCharacter::StopJumping()
{
	Super::StopJumping();

	//for all clones, duplicate input
	if (PlayerClones.Num() > 0)
	{
		for (ADecoherenceCharacter* Clone : PlayerClones)
		{
			Clone->StopJumping();
		}
	}
}

void ADecoherenceCharacter::Pause()
{
	ADecoherenceGameMode* GameMode = Cast<ADecoherenceGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	
	if (GameMode)
	{
		if (GameMode->IsPaused())
		{
			GameMode->UnPauseGame();
		}
		else
		{
			GameMode->PauseGame();
		}
	}
}
