// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeSplitter.h"
#include "Components/BoxComponent.h"
#include "DecoherenceCharacter.h"
#include "Curves/CurveVector.h"
#include "TimeMender.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

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

	//Set get current offset
	float CurveMin;
	float CurveMax;
	SplitOffsetVectorCurve->GetTimeRange(CurveMin, CurveMax);
	FVector CurrentOffset = SplitOffsetVectorCurve->GetVectorValue(CurveMax);

	//Set new root locations
	if (ActorsToDupe.Num() > 0 && DupedActors.Num() > 0)
	{
		OriginalRootLocation = ActorsToDupe[0]->GetActorLocation() - CurrentOffset;
		DupeRootLocation = DupedActors[0]->GetActorLocation() + CurrentOffset;
	}
	
	OriginalPlayerLocation = Player->GetActorLocation() - CurrentOffset;
	OriginalCloneLocation = ClonedPlayer->GetActorLocation() + CurrentOffset;

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

	//Set get current offset
	float CurveMin;
	float CurveMax;
	SplitOffsetVectorCurve->GetTimeRange(CurveMin, CurveMax);
	FVector CurrentOffset = SplitOffsetVectorCurve->GetVectorValue(CurveMax);

	//Set new root locations of non-player actors
	if (ActorsToDupe.Num() > 0 && DupedActors.Num() > 0)
	{
		OriginalRootLocation = ActorsToDupe[0]->GetActorLocation() - CurrentOffset;
		DupeRootLocation = DupedActors[0]->GetActorLocation() + CurrentOffset;
	}

	bTimeIsResetting = true;

	/* Move the new root actors by their offset via the offset timeline. Re-enables physics states when done*/
	SplitTimelineComp->Reverse();
}

// Called when the game starts or when spawned
void ATimeSplitter::BeginPlay()
{
	Super::BeginPlay();

	//Bind offset track to correlating function
	UpdateOffsetTrack.BindDynamic(this, &ATimeSplitter::UpdateActorsOffsets);

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
		if (Actor->GetRootComponent()->IsSimulatingPhysics())
		{
			Actor->Tags.Add(TEXT("EnablePhysics"));
		}
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		if (Actor->GetRootComponent()->IsSimulatingPhysics())
		{
			Actor->Tags.Add(TEXT("EnablePhysics"));
		}
	}
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		if (Actor->GetRootComponent()->IsSimulatingPhysics())
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
	for (AActor* Actor : DupedActors)
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
	for (AActor* Actor : ActorsStayWithDupes)
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
	for (AActor* Actor : ActorsStayWithOriginals)
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

void ATimeSplitter::DupeActors()
{
	/* Duplicate each actor and add to dupe array */
	for (AActor* OriginalActor : ActorsToDupe)
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

void ATimeSplitter::ClonePlayer()
{
	//Clone player
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ClonedPlayer = GetWorld()->SpawnActor<ADecoherenceCharacter>(CloneCharacterClass, Player->GetActorLocation(), Player->GetActorRotation(), SpawnParams);
	if (ClonedPlayer && Player)
	{
		ClonedPlayer->SetOwner(Player);
		Player->AddClone(ClonedPlayer);
		//Store original location of player for relative translation afterwards
		OriginalPlayerLocation = Player->GetActorLocation();
		OriginalCloneLocation = ClonedPlayer->GetActorLocation();
	}
}

void ATimeSplitter::AttachActorsToRoots()
{
	/* Once duplicated, attach all actors in each array to the first actor IF array is larger than 1 actor*/
	if (ActorsToDupe.Num() > 1)
	{
		RootOfOriginals = ActorsToDupe[0];

		for (int i = 1; i < ActorsToDupe.Num(); i++)
		{
			ActorsToDupe[i]->AttachToActor(RootOfOriginals, FAttachmentTransformRules::KeepWorldTransform);
		}

		//Store original location of root actor for relative translation afterwards
		OriginalRootLocation = RootOfOriginals->GetActorLocation();
	}

	if (DupedActors.Num() > 1)
	{
		RootOfDupes = DupedActors[0];

		for (int i = 1; i < DupedActors.Num(); i++)
		{
			DupedActors[i]->AttachToActor(RootOfDupes, FAttachmentTransformRules::KeepWorldTransform);
		}

		//Store original location of root actor for relative translation afterwards
		DupeRootLocation = RootOfDupes->GetActorLocation();
	}

	//Attach non-dupeing actors to respective roots
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		bool AttachSucc = Actor->AttachToActor(RootOfOriginals, FAttachmentTransformRules::KeepWorldTransform);
		UE_LOG(LogTemp, Display, TEXT("Attach to originals result: %d"), AttachSucc);
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		bool AttachSucc = Actor->AttachToActor(RootOfDupes, FAttachmentTransformRules::KeepWorldTransform);
		UE_LOG(LogTemp, Display, TEXT("Attach to dupes result: %d"), AttachSucc);
	}
}

void ATimeSplitter::DetachActorsFromRoots()
{
	/* Detach all actors in each array from the first actor IF array is larger than 1 actor*/
	if (ActorsToDupe.Num() > 1)
	{
		RootOfOriginals = ActorsToDupe[0];
		

		//Store original location of root actor for relative translation afterwards
		OriginalRootLocation = RootOfOriginals->GetActorLocation();

		for (int i = 1; i < ActorsToDupe.Num(); i++)
		{
			ActorsToDupe[i]->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	if (DupedActors.Num() > 1)
	{
		RootOfDupes = DupedActors[0];

		//Store original location of root actor for relative translation afterwards
		DupeRootLocation = RootOfDupes->GetActorLocation();

		for (int i = 1; i < DupedActors.Num(); i++)
		{
			DupedActors[i]->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	//Attach non-dupeing actors to respective roots
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void ATimeSplitter::ReenableActors()
{
	//Re-enable physics and collision for actors
	for (AActor* Actor : ActorsToDupe)
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
	for (AActor* Actor : DupedActors)
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
	for (AActor* Actor : ActorsStayWithDupes)
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
	for (AActor* Actor : ActorsStayWithOriginals)
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

void ATimeSplitter::DestroyDesignatedActors()
{
	if (!bTimeIsResetting)
	{
		for (AActor* Actor : ActorsWontMakeIt)
		{
			Actor->Destroy();
		}
	}
	for (AActor* Actor : DupedActors)
	{
		Actor->Destroy();
	}
	Player->RemoveClone(ClonedPlayer);
	ClonedPlayer->Destroy();
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

void ATimeSplitter::RecordActorResetTransforms()
{
	for (AActor* Actor : ActorsToDupe)
	{
		ActorsToDupeResetTransforms.Add(Actor->GetActorTransform());
	}
	for (AActor* Actor : DupedActors)
	{
		DupedActorsResetTransforms.Add(Actor->GetActorTransform());
	}
	for (AActor* Actor : ActorsStayWithDupes)
	{
		StayWithOriginalsResetTransforms.Add(Actor->GetActorTransform());
	}
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		StayWithDupesResetTransforms.Add(Actor->GetActorTransform());
	}
}

void ATimeSplitter::MoveActorsToResetTransform()
{	
	int32 index = 0;
	for (AActor* Actor : ActorsToDupe)
	{
		Actor->SetActorTransform(ActorsToDupeResetTransforms[index]);
		index++;
	}
	index = 0;
	for (AActor* Actor : DupedActors)
	{
		Actor->SetActorTransform(DupedActorsResetTransforms[index]);
		index++;
	}
	index = 0;
	for (AActor* Actor : ActorsStayWithDupes)
	{
		Actor->SetActorTransform(StayWithOriginalsResetTransforms[index]);
		index++;
	}
	index = 0;
	for (AActor* Actor : ActorsStayWithOriginals)
	{
		Actor->SetActorTransform(StayWithDupesResetTransforms[index]);
		index++;
	}
}

void ATimeSplitter::ClearDupedActors()
{
	for (AActor* Actor : DupedActors)
	{
		Actor->Destroy();
	}
	DupedActors.Empty();
}

