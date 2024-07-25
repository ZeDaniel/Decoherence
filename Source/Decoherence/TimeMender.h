// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimeMender.generated.h"

UCLASS()
class DECOHERENCE_API ATimeMender : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATimeMender();

	void ActivateMender();
	void DeactivateMender();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	bool AttemptToMend();

	UPROPERTY(EditAnywhere, Category = "Time Split")
	class ATimeSplitter* TimeSplitter;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	bool IsActive = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
