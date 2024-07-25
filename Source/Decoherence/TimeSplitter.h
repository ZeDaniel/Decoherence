// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "TimeSplitter.generated.h"


UCLASS()
class DECOHERENCE_API ATimeSplitter : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATimeSplitter();

	void UnSplit();

	void ResetSplit();

protected:

	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* BoxComp;
	UPROPERTY(VisibleAnywhere)
	class UBoxComponent* ExitBoxComp;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	class UTimelineComponent* SplitTimelineComp;

	/** Offset curve that determines motion of the split actors*/
	UPROPERTY(EditAnywhere, Category = "Time Split")
	UCurveVector* SplitOffsetVectorCurve;

	/** Input mapping context to give to player for split time*/
	UPROPERTY(EditAnywhere, Category = "Time Split")
	class UInputMappingContext* SplitTimeMappingContext;

	/** Input action to give to player for split time*/
	UPROPERTY(EditAnywhere, Category = "Time Split")
	class UInputAction* ResetSplitAction;


	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Code for when something ends overlap with the root box*/
	UFUNCTION()
	void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	/** Code for when something overlaps the exit box */
	UFUNCTION()
	void OnExitBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	/** Code for when something ends overlap of the exit box */
	UFUNCTION()
	void OnExitBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SplitTime();
	/* Preserves the physics states of all actors*/
	void PreserveActorsPhysicsStates();
	/* Temporarily disables the physics states of all actors*/
	void DisableActors();
	/* Creates a duplicate actor for every actor in ActorsToDupe and adds them to DupedActors*/
	void DupeActors();
	/* Creates a clone of the player and adds the clone to the player's clone array*/
	void ClonePlayer();
	/* Attaches original, duped, and non-duping actors to their respective roots*/
	void AttachActorsToRoots();
	/* Detaches original, duped, and non-duping actors to their respective roots*/
	void DetachActorsFromRoots();
	/* Re-enables the  actors*/
	void ReenableActors();
	/* Destroys designated actors upon mend*/
	void DestroyDesignatedActors();

	/* Associated Time Mender. i.e. trigger system for un-splitting */
	UPROPERTY(EditAnywhere, Category = "Time Split")
	class ATimeMender* TimeMender;

	/** Array of actors to duplicate for time split */
	UPROPERTY(EditAnywhere, Category = "Time Split")
	TArray<AActor*> ActorsToDupe;
	/** Array to hold duped actors*/
	TArray<AActor*> DupedActors;
	

	/** Array of actors to move with original group. Not duping */
	UPROPERTY(EditAnywhere, Category = "Time Split")
	TArray<AActor*> ActorsStayWithOriginals;

	/** Array of actors to move with dupe group. Not duping */
	UPROPERTY(EditAnywhere, Category = "Time Split")
	TArray<AActor*> ActorsStayWithDupes;

	/** Array of actors that will not survive the merge */
	UPROPERTY(EditAnywhere, Category = "Time Split")
	TArray<AActor*> ActorsWontMakeIt;

	FOnTimelineVector UpdateOffsetTrack;

	FOnTimelineEvent ReenableActorsEvent;
	FOnTimelineEvent DestroyDesignatedActorsEvent;

	/* Move actors according to the movement curve*/
	UFUNCTION()
	void UpdateActorsOffsets(FVector OffsetOutput);
	/* Re-enable all disabled actors*/
	UFUNCTION()
	void EndTimelineDelegate();
	/* Destroy actors designated in the ActorsWontMakeIt array*/
	UFUNCTION()
	void EndReverseTimelineDelegate();

	UPROPERTY(EditAnywhere, Category = "Time Split")
	TSubclassOf<class ADecoherenceCharacter> CloneCharacterClass;

private:
	bool bGateActive;

	bool bPlayerOnExitSide = false;

	FVector OriginalRootLocation;
	FVector DupeRootLocation;
	AActor* RootOfOriginals;
	AActor* RootOfDupes;

	FVector OriginalPlayerLocation;
	FVector OriginalCloneLocation;
	class ADecoherenceCharacter* Player;
	class ADecoherenceCharacter* ClonedPlayer;

	bool bTimeIsSplit = false;
	bool bTimeIsResetting = false;
	void UpdateSplitStatus();

	TArray<FTransform> ActorsToDupeResetTransforms;
	TArray<FTransform> DupedActorsResetTransforms;
	TArray<FTransform> StayWithOriginalsResetTransforms;
	TArray<FTransform> StayWithDupesResetTransforms;

	void RecordActorResetTransforms();
	void MoveActorsToResetTransform();
	void ClearDupedActors();

	void AddSplitTimeMappingContext();
	void RemoveSplitTimeMappingContext();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
