// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeSplitter.h"
#include "Components/BoxComponent.h"
#include "DecoherenceCharacter.h"
#include "Curves/CurveVector.h"
#include "TimeMender.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ATimeSplitter::ATimeSplitter()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Box Component"));
	SetRootComponent(BoxComp);

	ExitBoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("Exit Box Component"));
	ExitBoxComp->SetupAttachment(RootComponent);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh Component"));
	MeshComp->SetupAttachment(RootComponent);

	SplitTimelineComp = CreateDefaultSubobject<UTimelineComponent>(TEXT("Split Timeline Component"));

}

void ATimeSplitter::UnSplit()
{
	//disable physics states before merge
	DisableActors();

	//Re-attach assets to the roots. Specifically because physics actors need to be reattached
	AttachActorsToRoots();

	//Set player locations
	if(SplitOffsetVectorCurve)
		SetPlayerLocations();

	//Set current actor rotations
	if(SplitActorRotationVectorCurve)
		SetActorRotations();

	/* Move the new root actors by their offset via the offset timeline. Re-enables physics states when done*/
	SplitTimelineComp->Reverse();
}

void ATimeSplitter::ResetSplit()
{
	//disable physics states before merge
	DisableActors();

	//Move actors to reset transform
	MoveActorsToResetTransform();

	//Re-attach assets to the roots. Specifically because physics actors need to be reattached
	AttachActorsToRoots();

	bTimeIsResetting = true;

	//Set current actor rotations
	if (SplitActorRotationVectorCurve)
		SetActorRotations();

	/* Move the new root actors by their offset via the offset timeline. Re-enables physics states when done*/
	SplitTimelineComp->Reverse();
}

// Called when the game starts or when spawned
void ATimeSplitter::BeginPlay()
{
	Super::BeginPlay();

	//Bind offset track to correlating function
	UpdateOffsetTrack.BindDynamic(this, &ATimeSplitter::UpdateActorsOffsets);

	//Bind rotation track to correlating function
	UpdateActorRotationTrack.BindDynamic(this, &ATimeSplitter::UpdateActorsRotations);

	//Bind rotation track to correlating function
	UpdateCloneRotationTrack.BindDynamic(this, &ATimeSplitter::UpdateCloneRotations);

	//Bind re-enable actors event to correlating function
	ReenableActorsEvent.BindDynamic(this, &ATimeSplitter::EndTimelineDelegate);

	//Bind destroy actors event to correlating function
	DestroyDesignatedActorsEvent.BindDynamic(this, &ATimeSplitter::EndReverseTimelineDelegate);

	//If vector curve isn't null, bind the graph to the update function
	if (SplitOffsetVectorCurve)
	{
		SplitTimelineComp->AddInterpVector(SplitOffsetVectorCurve, UpdateOffsetTrack);

		float CurveMin;
		float CurveMax;
		SplitOffsetVectorCurve->GetTimeRange(CurveMin, CurveMax);

		SplitTimelineComp->AddEvent(CurveMax, ReenableActorsEvent);
		SplitTimelineComp->AddEvent(CurveMin, DestroyDesignatedActorsEvent);
	}

	//If rotation curve isn't null, bind the graph to the update function
	if (SplitActorRotationVectorCurve)
	{
		SplitTimelineComp->AddInterpVector(SplitActorRotationVectorCurve, UpdateActorRotationTrack);
	}

	//If rotation curve isn't null, bind the graph to the update function
	if (SplitCloneRotationVectorCurve)
	{
		SplitTimelineComp->AddInterpVector(SplitCloneRotationVectorCurve, UpdateCloneRotationTrack);
	}
	

	// Register our Overlap Events
	BoxComp->OnComponentEndOverlap.AddDynamic(this, &ATimeSplitter::OnBoxEndOverlap);

	ExitBoxComp->OnComponentBeginOverlap.AddDynamic(this, &ATimeSplitter::OnExitBoxBeginOverlap);
	ExitBoxComp->OnComponentEndOverlap.AddDynamic(this, &ATimeSplitter::OnExitBoxEndOverlap);

	bGateActive = true;
}

void ATimeSplitter::OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//Check gate is active, ignore if not
	if (bGateActive)
	{
		//If other actor is a Decoherence Character, then trigger and split time
		if (ADecoherenceCharacter* Character = Cast<ADecoherenceCharacter>(OtherActor))
		{
			//Check player is on exit side of gate
			if (bPlayerOnExitSide)
			{
				// Save character to dupe during splittime
				if (Character->ActorHasTag("Player"))
				{
					bGateActive = false;
					Player = Character;
				}
				SplitTime();
			}
		}
	}
}

void ATimeSplitter::OnExitBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//If other actor is a Decoherence Character, then trigger and split time
	if (ADecoherenceCharacter* Character = Cast<ADecoherenceCharacter>(OtherActor))
	{
		bPlayerOnExitSide = true;
	}
}

void ATimeSplitter::OnExitBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//If other actor is a Decoherence Character, then trigger and split time
	if (ADecoherenceCharacter* Character = Cast<ADecoherenceCharacter>(OtherActor))
	{
		bPlayerOnExitSide = false;
	}
}

void ATimeSplitter::SplitTime()
{
	if (TimeMender)
	{
		if (ActorsToDupe.Num() > 0)
		{
			//Preserve and disable physics states before dupe
			PreserveActorsPhysicsStates();
			DisableActors();

			DupeActors();

			ClonePlayer();

			AttachActorsToRoots();

			//Set current actor rotations
			if (SplitActorRotationVectorCurve)
				SetActorRotations();

			/* Move the new root actors by their offset via the offset timeline. Re-enables physics states when done*/
			SplitTimelineComp->Play();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Split Failed. Time Mender not present"));
	}
	
}

void ATimeSplitter::PreserveActorsPhysicsStates()
{
	//Add tag to every actor component simulating physics to remember them
	for (AActor* Actor : ActorsToDupe)
	{
		if (Actor && Actor->GetRootComponent()->IsSimulatingPhysics())
		{
			Actor->Tags.Add(TEXT("EnablePhysics"));
		}
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		if (Actor && Actor->GetRootComponent()->IsSimulatingPhysics())
		{
			Actor->Tags.Add(TEXT("EnablePhysics"));
		}
	}
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		if (Actor && Actor->GetRootComponent()->IsSimulatingPhysics())
		{
			Actor->Tags.Add(TEXT("EnablePhysics"));
		}
	}
}

void ATimeSplitter::DisableActors()
{
	//disable physics for every actor simulating physics
	for (AActor* Actor : ActorsToDupe)
	{
		if (Actor)
		{
			Actor->SetActorEnableCollision(false);

			if (Actor->ActorHasTag(TEXT("EnablePhysics")))
			{
				UPrimitiveComponent* ActorPhysicsRoot = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
				if (ActorPhysicsRoot)
				{
					ActorPhysicsRoot->SetSimulatePhysics(false);
				}
			}
		}
	}
	for (AActor* Actor : DupedActors)
	{
		if (Actor)
		{
			Actor->SetActorEnableCollision(false);

			if (Actor->ActorHasTag(TEXT("EnablePhysics")))
			{
				UPrimitiveComponent* ActorPhysicsRoot = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
				if (ActorPhysicsRoot)
				{
					ActorPhysicsRoot->SetSimulatePhysics(false);
				}
			}
		}
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		if (Actor)
		{
			Actor->SetActorEnableCollision(false);

			if (Actor->ActorHasTag(TEXT("EnablePhysics")))
			{
				UPrimitiveComponent* ActorPhysicsRoot = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
				if (ActorPhysicsRoot)
				{
					ActorPhysicsRoot->SetSimulatePhysics(false);
				}
			}
		}
	}
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		if (Actor)
		{
			Actor->SetActorEnableCollision(false);

			if (Actor->ActorHasTag(TEXT("EnablePhysics")))
			{
				UPrimitiveComponent* ActorPhysicsRoot = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
				if (ActorPhysicsRoot)
				{
					ActorPhysicsRoot->SetSimulatePhysics(false);
				}
			}
		}
	}

	if (Player)
	{
		Player->SetActorEnableCollision(false);
		APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
		if (PlayerController)
		{
			Player->DisableInput(PlayerController);
		}
	}

	if (ClonedPlayer)
	{
		ClonedPlayer->SetActorEnableCollision(false);
	}
}

void ATimeSplitter::DupeActors()
{
	/* Duplicate each actor and add to dupe array */
	for (AActor* OriginalActor : ActorsToDupe)
	{
		if (OriginalActor)
		{
			//Set values for the spawn
			FActorSpawnParameters SpawnParams;
			SpawnParams.Template = OriginalActor;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			//Spawn dupe actor
			AActor* DupedActor = GetWorld()->SpawnActor<AActor>(OriginalActor->GetClass(), SpawnParams);
			if (DupedActor)
			{
				DupedActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
				DupedActors.Add(DupedActor);
			}
		}
	}
}

void ATimeSplitter::ClonePlayer()
{
	//Clone player
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ClonedPlayer = GetWorld()->SpawnActor<ADecoherenceCharacter>(CloneCharacterClass, Player->GetActorLocation(), Player->GetActorRotation(), SpawnParams);
	if (ClonedPlayer && Player)
	{
		//Drop all movement for player and clone
		Player->GetCharacterMovement()->StopMovementImmediately();
		ClonedPlayer->GetCharacterMovement()->StopMovementImmediately();

		ClonedPlayer->SetOwner(Player);
		Player->AddClone(ClonedPlayer);
		//Store original location of player for relative translation afterwards
		OriginalPlayerLocation = Player->GetActorLocation();
		OriginalCloneLocation = ClonedPlayer->GetActorLocation();
	}
}

void ATimeSplitter::AttachActorsToRoots()
{
	bool RootsFound = SetActorRoots();

	if (RootsFound)
	{
		/* Once duplicated, attach all actors in each array to the first actor IF array is larger than 1 actor*/
		if (ActorsToDupe.Num() > 1)
		{
			for (int i = 1; i < ActorsToDupe.Num(); i++)
			{
				if (ActorsToDupe[i])
				{
					ActorsToDupe[i]->AttachToActor(RootOfOriginals, FAttachmentTransformRules::KeepWorldTransform);
				}
			}
		}

		if (DupedActors.Num() > 1)
		{
			for (int i = 1; i < DupedActors.Num(); i++)
			{
				if (DupedActors[i])
				{
					DupedActors[i]->AttachToActor(RootOfDupes, FAttachmentTransformRules::KeepWorldTransform);
				}
			}
		}
	}
	

	//Attach non-dupeing actors to respective roots
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		if (Actor)
		{
			bool AttachSucc = Actor->AttachToActor(RootOfOriginals, FAttachmentTransformRules::KeepWorldTransform);
			UE_LOG(LogTemp, Display, TEXT("Attach to originals result: %d"), AttachSucc);
		}
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		if (Actor)
		{
			bool AttachSucc = Actor->AttachToActor(RootOfDupes, FAttachmentTransformRules::KeepWorldTransform);
			UE_LOG(LogTemp, Display, TEXT("Attach to dupes result: %d"), AttachSucc);
		}
	}
}

void ATimeSplitter::DetachActorsFromRoots()
{
	bool RootsFound = SetActorRoots();

	if (RootsFound)
	{
		/* Detach all actors in each array from the first actor IF array is larger than 1 actor*/
		if (ActorsToDupe.Num() > 1)
		{
			//Store original location of root actor for relative translation afterwards
			OriginalRootLocation = RootOfOriginals->GetActorLocation();

			for (int i = 1; i < ActorsToDupe.Num(); i++)
			{
				if (ActorsToDupe[i])
				{
					ActorsToDupe[i]->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				}
			}
		}

		if (DupedActors.Num() > 1)
		{
			//Store original location of root actor for relative translation afterwards
			DupeRootLocation = RootOfDupes->GetActorLocation();

			for (int i = 1; i < DupedActors.Num(); i++)
			{
				if (DupedActors[i])
				{
					DupedActors[i]->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				}
			}
		}

		//Attach non-dupeing actors to respective roots
		for (AActor* Actor : ActorsStayWithOriginals)
		{
			if (Actor)
			{
				Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			}
		}
		for (AActor* Actor : ActorsStayWithDupes)
		{
			if (Actor)
			{
				Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			}
		}
	}
}

void ATimeSplitter::ReenableActors()
{
	//Re-enable physics and collision for actors
	for (AActor* Actor : ActorsToDupe)
	{
		if (Actor)
		{
			Actor->SetActorEnableCollision(true);

			if (Actor->ActorHasTag(TEXT("EnablePhysics")))
			{
				UPrimitiveComponent* ActorPhysicsRoot = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
				if (ActorPhysicsRoot)
				{
					ActorPhysicsRoot->SetSimulatePhysics(true);
				}
			}
		}
	}
	for (AActor* Actor : DupedActors)
	{
		if (Actor)
		{
			Actor->SetActorEnableCollision(true);

			if (Actor->ActorHasTag(TEXT("EnablePhysics")))
			{
				UPrimitiveComponent* ActorPhysicsRoot = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
				if (ActorPhysicsRoot)
				{
					ActorPhysicsRoot->SetSimulatePhysics(true);
				}
			}
		}
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		if (Actor)
		{
			Actor->SetActorEnableCollision(true);

			if (Actor->ActorHasTag(TEXT("EnablePhysics")))
			{
				UPrimitiveComponent* ActorPhysicsRoot = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
				if (ActorPhysicsRoot)
				{
					ActorPhysicsRoot->SetSimulatePhysics(true);
				}
			}
		}
	}
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		if (Actor)
		{
			Actor->SetActorEnableCollision(true);

			if (Actor->ActorHasTag(TEXT("EnablePhysics")))
			{
				UPrimitiveComponent* ActorPhysicsRoot = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
				if (ActorPhysicsRoot)
				{
					ActorPhysicsRoot->SetSimulatePhysics(true);
				}
			}
		}
	}

	if (Player)
	{
		Player->SetActorEnableCollision(true);

		APlayerController* PlayerController = Cast<APlayerController>(Player->GetController());
		if (PlayerController)
		{
			Player->EnableInput(PlayerController);
		}
	}

	if (ClonedPlayer)
	{
		ClonedPlayer->SetActorEnableCollision(true);
	}
}

void ATimeSplitter::DestroyDesignatedActors()
{
	if (!bTimeIsResetting)
	{
		for (AActor* Actor : ActorsWontMakeIt)
		{
			if(Actor)
				Actor->Destroy();
		}
	}
	for (AActor* Actor : DupedActors)
	{
		if(Actor)
			Actor->Destroy();
	}
	if (ClonedPlayer)
	{
		Player->RemoveClone(ClonedPlayer);
		ClonedPlayer->Destroy();
		ClonedPlayer = nullptr;
	}
}

void ATimeSplitter::UpdateActorsOffsets(FVector OffsetOutput)
{
	if (RootOfOriginals && RootOfDupes)
	{
		RootOfOriginals->SetActorLocation(OriginalRootLocation + OffsetOutput);
		RootOfDupes->SetActorLocation(DupeRootLocation - OffsetOutput);
	}

	if (Player && ClonedPlayer)
	{
		Player->SetActorLocation(OriginalPlayerLocation + OffsetOutput);
		ClonedPlayer->SetActorLocation(OriginalCloneLocation - OffsetOutput);
	}
}

void ATimeSplitter::UpdateActorsRotations(FVector RotationOutput)
{
	if (RootOfOriginals && RootOfDupes)
	{ 
		FRotator NewRotation = OriginalRootRotation;
		NewRotation.Roll += RotationOutput.X;
		NewRotation.Pitch += RotationOutput.Y;
		NewRotation.Yaw += RotationOutput.Z;
		RootOfOriginals->SetActorRotation(NewRotation);

		NewRotation = DupeRootRotation;
		NewRotation.Roll -= RotationOutput.X;
		NewRotation.Pitch -= RotationOutput.Y;
		NewRotation.Yaw -= RotationOutput.Z;
		RootOfDupes->SetActorRotation(NewRotation);
	}
}

void ATimeSplitter::UpdateCloneRotations(FVector RotationOutput)
{
	if (ClonedPlayer)
	{
		FRotator NewRotation = OriginalCloneRotation;
		NewRotation.Roll -= RotationOutput.X;
		NewRotation.Pitch -= RotationOutput.Y;
		NewRotation.Yaw -= RotationOutput.Z;
		ClonedPlayer->SetActorRotation(NewRotation);
	}
}

void ATimeSplitter::EndTimelineDelegate()
{
	if (!bTimeIsSplit && !bTimeIsResetting)
	{
		RecordActorResetTransforms();
		ReenableActors();
		UpdateSplitStatus();
		bGateActive = false;
		TimeMender->ActivateMender();
	}
	AddSplitTimeMappingContext();
}

void ATimeSplitter::EndReverseTimelineDelegate()
{
	if (bTimeIsSplit && !bTimeIsResetting)
	{
		DetachActorsFromRoots();
		DestroyDesignatedActors();
		
		UpdateSplitStatus();
		ReenableActors();
		TimeMender->DeactivateMender();
	}
	else if (bTimeIsResetting)
	{
		ClearDupedActors();

		Player->RemoveClone(ClonedPlayer);
		ClonedPlayer->Destroy();
		ClonedPlayer = nullptr;

		DetachActorsFromRoots();

		UpdateSplitStatus();
		ReenableActors();

		bGateActive = true;
		TimeMender->DeactivateMender();

		bTimeIsResetting = false;
	}

	RemoveSplitTimeMappingContext();
}

void ATimeSplitter::UpdateSplitStatus()
{
	bTimeIsSplit = !bTimeIsSplit;
}

void ATimeSplitter::AddSplitTimeMappingContext()
{
	// Set up action bindings
	if (Player)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Player->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				// Set the priority of the mapping to 1
				Subsystem->AddMappingContext(SplitTimeMappingContext, 1);
			}

			if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
			{
				// Reset
				EnhancedInputComponent->BindAction(ResetSplitAction, ETriggerEvent::Triggered, this, &ATimeSplitter::ResetSplit);
			}
		}
	}
}

void ATimeSplitter::RemoveSplitTimeMappingContext()
{
	if (Player)
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(Player->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				// Remove mapping context
				Subsystem->RemoveMappingContext(SplitTimeMappingContext);
			}
		}
	}
}

// Called every frame
void ATimeSplitter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

bool ATimeSplitter::SetActorRoots()
{
	//Get current offset
	float CurveMin;
	float CurveMax;
	SplitOffsetVectorCurve->GetTimeRange(CurveMin, CurveMax);
	FVector CurrentOffset;

	if (bTimeIsSplit)
	{
		CurrentOffset = SplitOffsetVectorCurve->GetVectorValue(CurveMax);
	}
	else
	{
		CurrentOffset = SplitOffsetVectorCurve->GetVectorValue(CurveMin);
	}

	//Set new root locations of non-player actors

	bool OriginalRootFound = false;
	bool DupeRootFound = false;

	if (ActorsToDupe.Num() > 0)
	{
		int32 index = 0;

		while (!OriginalRootFound  && (ActorsToDupe.Num() > index))
		{
			if (ActorsToDupe[index])
			{
				RootOfOriginals = ActorsToDupe[index];
				OriginalRootLocation = RootOfOriginals->GetActorLocation() - CurrentOffset;
				OriginalRootFound = true;
			}
			index++;
		}

		if(!OriginalRootFound)
			UE_LOG(LogTemp, Warning, TEXT("Root not found for originals"));
	}

	if (DupedActors.Num() > 0)
	{
		int32 index = 0;

		while (!DupeRootFound && (DupedActors.Num() > index))
		{
			if (DupedActors[index])
			{
				RootOfDupes = DupedActors[index];
				DupeRootLocation = RootOfDupes->GetActorLocation() + CurrentOffset;
				DupeRootFound = true;
			}
			index++;
		}

		if (!DupeRootFound)
			UE_LOG(LogTemp, Warning, TEXT("Root not found for dupes"));
	}

	if (!OriginalRootFound)
		return false;
	else
		return true;
}

void ATimeSplitter::SetPlayerLocations()
{
	//Get current offset
	float CurveMin;
	float CurveMax;
	SplitOffsetVectorCurve->GetTimeRange(CurveMin, CurveMax);
	FVector CurrentOffset;

	if (bTimeIsSplit)
	{
		CurrentOffset = SplitOffsetVectorCurve->GetVectorValue(CurveMax);
	}
	else
	{
		CurrentOffset = SplitOffsetVectorCurve->GetVectorValue(CurveMin);
	}

	if (Player)
	{
		OriginalPlayerLocation = Player->GetActorLocation() - CurrentOffset;
	}

	if (ClonedPlayer)
	{
		OriginalCloneLocation = ClonedPlayer->GetActorLocation() + CurrentOffset;
	}
}

void ATimeSplitter::SetActorRotations()
{
	//Get current offset
	float CurveMin;
	float CurveMax;
	SplitOffsetVectorCurve->GetTimeRange(CurveMin, CurveMax);
	FVector CurrentOffset;

	if (bTimeIsSplit)
	{
		CurrentOffset = SplitActorRotationVectorCurve->GetVectorValue(CurveMax);
	}
	else
	{
		CurrentOffset = SplitActorRotationVectorCurve->GetVectorValue(CurveMin);
	}

	if (RootOfOriginals)
	{
		OriginalRootRotation = RootOfOriginals->GetActorRotation();
		OriginalRootRotation.Roll -= CurrentOffset.X;
		OriginalRootRotation.Pitch -= CurrentOffset.Y;
		OriginalRootRotation.Yaw -= CurrentOffset.Z;
	}

	if (RootOfDupes)
	{
		DupeRootRotation = RootOfDupes->GetActorRotation();
		DupeRootRotation.Roll += CurrentOffset.X;
		DupeRootRotation.Pitch += CurrentOffset.Y;
		DupeRootRotation.Yaw += CurrentOffset.Z;
	}

	if (Player)
	{
		OriginalPlayerRotation = Player->GetActorRotation();
		OriginalPlayerRotation.Roll -= CurrentOffset.X;
		OriginalPlayerRotation.Pitch -= CurrentOffset.Y;
		OriginalPlayerRotation.Yaw -= CurrentOffset.Z;
	}

	if (ClonedPlayer)
	{
		OriginalCloneRotation = ClonedPlayer->GetActorRotation();
		OriginalCloneRotation.Roll += CurrentOffset.X;
		OriginalCloneRotation.Pitch += CurrentOffset.Y;
		OriginalCloneRotation.Yaw += CurrentOffset.Z;
	}
}


void ATimeSplitter::RecordActorResetTransforms()
{
	for (AActor* Actor : ActorsToDupe)
	{
		if (Actor)
		{
			ActorsToDupeResetTransforms.Add(Actor->GetActorTransform());
		}
	}
	for (AActor* Actor : DupedActors)
	{
		if (Actor)
		{
			DupedActorsResetTransforms.Add(Actor->GetActorTransform());
		}
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		if (Actor)
		{
			StayWithOriginalsResetTransforms.Add(Actor->GetActorTransform());
		}
	}
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		if (Actor)
		{
			StayWithDupesResetTransforms.Add(Actor->GetActorTransform());
		}
	}
}

void ATimeSplitter::MoveActorsToResetTransform()
{	
	int32 index = 0;
	for (AActor* Actor : ActorsToDupe)
	{
		if (Actor)
		{
			Actor->SetActorTransform(ActorsToDupeResetTransforms[index]);
			index++;
		}
	}
	index = 0;
	for (AActor* Actor : DupedActors)
	{
		if (Actor)
		{
			Actor->SetActorTransform(DupedActorsResetTransforms[index]);
			index++;
		}
	}
	index = 0;
	for (AActor* Actor : ActorsStayWithDupes)
	{
		if (Actor)
		{
			Actor->SetActorTransform(StayWithOriginalsResetTransforms[index]);
			index++;
		}
	}
	index = 0;
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		if (Actor)
		{
			Actor->SetActorTransform(StayWithDupesResetTransforms[index]);
			index++;
		}
	}

	if (Player)
		Player->SetActorLocation(OriginalPlayerLocation);
	if (ClonedPlayer)
		ClonedPlayer->SetActorLocation(OriginalCloneLocation);
}

void ATimeSplitter::ClearDupedActors()
{
	for (AActor* Actor : DupedActors)
	{
		Actor->Destroy();
	}
	DupedActors.Empty();
}

