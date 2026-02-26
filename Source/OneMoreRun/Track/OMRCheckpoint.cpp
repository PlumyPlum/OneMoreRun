// Fill out your copyright notice in the Description page of Project Settings.


#include "OMRCheckpoint.h"
#include "../Game/OMRTimeTrialGameState.h"
#include "../Player/OMRPlayerPawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AOMRCheckpoint::AOMRCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);

	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
}

void AOMRCheckpoint::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AOMRCheckpoint::OnTriggerBeginOverlap);
}

void AOMRCheckpoint::ResetCheckpoint()
{
	if (!Trigger) return;

	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetGenerateOverlapEvents(true);
}

void AOMRCheckpoint::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->IsA(AOMRPlayerPawn::StaticClass()))
	{
		return;
	}

	AOMRTimeTrialGameState* GS = GetWorld() ? GetWorld()->GetGameState<AOMRTimeTrialGameState>() : nullptr;
	if (!GS) return;

	GS->RegisterCheckpointHit(CheckpointIndex, GetActorTransform());

	// Prevent double firing this lap
	Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}



