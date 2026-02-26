// Fill out your copyright notice in the Description page of Project Settings.


#include "OMRStartFinishGate.h"
#include "Components/BoxComponent.h"
#include "../Game/OMRTimeTrialGameState.h"

// Sets default values
AOMRStartFinishGate::AOMRStartFinishGate()
{
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;

	Trigger->SetCollisionProfileName(TEXT("Trigger"));
}

// Called when the game starts or when spawned
void AOMRStartFinishGate::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("Gate BeginPlay Called"));

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AOMRStartFinishGate::OnOverlapBegin);
}

void AOMRStartFinishGate::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Gate Overlap Triggered"));

	if (!OtherActor) return;

	AOMRTimeTrialGameState* GS = GetWorld()->GetGameState<AOMRTimeTrialGameState>();

	if (!GS) return;

	GS->HandleStartFinishCross();
}


