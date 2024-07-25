// Fill out your copyright notice in the Description page of Project Settings.


#include "TimeMender.h"
#include "TimeSplitter.h"

// Sets default values
ATimeMender::ATimeMender()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void ATimeMender::ActivateMender()
{
	IsActive = true;
}

void ATimeMender::DeactivateMender()
{
	IsActive = false;
}

// Called when the game starts or when spawned
void ATimeMender::BeginPlay()
{
	Super::BeginPlay();
	
}

bool ATimeMender::AttemptToMend()
{
	if (IsActive && TimeSplitter)
	{
		TimeSplitter->UnSplit();
		IsActive = false;

		return true;
	}

	return false;
}

// Called every frame
void ATimeMender::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

