// Fill out your copyright notice in the Description page of Project Settings.


#include "OMRTimeTrialGameState.h"
#include "Kismet/GameplayStatics.h"
#include "../Track/OMRCheckpoint.h"

AOMRTimeTrialGameState::AOMRTimeTrialGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AOMRTimeTrialGameState::BeginPlay()
{
	Super::BeginPlay();

	CacheCheckpoints();
}

void AOMRTimeTrialGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateLapTimer(DeltaTime);

}

void AOMRTimeTrialGameState::StartLap()
{
	if (bLapActive)	return;
	
	bLapActive = true;
	CurrentLap++;
	LapStartTime = GetWorld()->GetTimeSeconds();

	// 🔥 Broadcast lap number change
	OnLapNumberUpdated.Broadcast(CurrentLap);

	UE_LOG(LogTemp, Warning, TEXT("Lap %d Started"), CurrentLap);
}

void AOMRTimeTrialGameState::CompleteLap()
{
	if (!bLapActive) return;

	float Now = GetWorld()->GetTimeSeconds();
	CurrentLapTime = Now - LapStartTime;

	// 🔥 Broadcast final lap time
	OnLapTimeUpdated.Broadcast(CurrentLapTime);

	bool bNewBest = false;

	if (BestLapTime < 0.f || CurrentLapTime < BestLapTime)
	{
		BestLapTime = CurrentLapTime;
		bNewBest = true;
	}

	bLapActive = false;

	UE_LOG(LogTemp, Warning, TEXT("Lap %d Complete - Time: %.2f | Best: %.2f"), CurrentLap, CurrentLapTime, BestLapTime);

	if (bNewBest)
	{
		BestSplitTimes = SplitTimes;
		UE_LOG(LogTemp, Warning, TEXT("New Best Lap!"));
		OnBestTimeUpdated.Broadcast(BestLapTime);
	}

}

void AOMRTimeTrialGameState::UpdateLapTimer(float DeltaTime)
{
	if (!bLapActive) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds() - LapStartTime;

	LapTimeBroadcastAccumulator += DeltaTime;

	if (LapTimeBroadcastAccumulator >= 0.05f)
	{
		LapTimeBroadcastAccumulator -= 0.05f;
		LastBroadcastLapTime = CurrentTime;

		OnLapTimeUpdated.Broadcast(CurrentTime);
	}
}

void AOMRTimeTrialGameState::HandleStartFinishCross()
{
	if (!GetWorld()) return;

	const float Now = GetWorld()->GetTimeSeconds();

	// Cooldown Protection
	if (Now - LastGateCrossTime < GateCooldown)
	{
		return;
	}
	
	LastGateCrossTime = Now;

	if(!bLapActive)
	{
		ResetCheckpoints();
		StartLap();
		return;
	}
	
	// Finish Logic
	const bool bHasCheckpointConfigured = (TotalCheckpoints > 0);
	const bool bAllCheckpointsCleared = (CurrentCheckpointIndex >= TotalCheckpoints);

	const bool bCanFinish =
		!bRequireCheckpointsToFinish ||
		!bHasCheckpointConfigured ||
		bAllCheckpointsCleared;

	if (bCanFinish)
	{
		CompleteLap();
		ResetCheckpoints();
		StartLap();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot finish lap. Missing checkpoints (%d/%d)"),
			CurrentCheckpointIndex, TotalCheckpoints);
	}
}

float AOMRTimeTrialGameState::GetDisplayedLaptime() const
{
	if (!GetWorld()) return CurrentLapTime;

	if (bLapActive)
	{
		return GetWorld()->GetTimeSeconds() - LapStartTime;
	}

	return CurrentLapTime;
}

void AOMRTimeTrialGameState::RegisterCheckpointHit(int32 CheckpointIndex, const FTransform& CheckpointTransform)
{
	if (!bLapActive) return;

	if (TotalCheckpoints <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Checkpoint hit but TotalCheckpoints is 0. Set it in editor."));
		return;
	}

	if (CheckpointIndex != CurrentCheckpointIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("Checkpoint %d ignored (expected %d)"), CheckpointIndex, CurrentCheckpointIndex);
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const float SplitTime = Now - LapStartTime;

	SplitTimes.Add(SplitTime);

	float SplitDelta = 0.f;
	bool bIsAhead;
	bool bHasReference = false;

	if (BestSplitTimes.IsValidIndex(CheckpointIndex))
	{
		SplitDelta = SplitTime - BestSplitTimes[CheckpointIndex];
		bIsAhead = SplitDelta < 0.f;
		bHasReference = true;
	}

	if (bHasReference)
	{
		OnSplitUpdated.Broadcast(SplitTime, SplitDelta, bIsAhead);
	}

	// Save respawn transform
	LastCheckpointTransform = CheckpointTransform;

	UE_LOG(LogTemp, Warning, TEXT("Checkpoint %d Hit | Split: %.2f"), CheckpointIndex, SplitTime);

	CurrentCheckpointIndex++;
}

void AOMRTimeTrialGameState::ResetCheckpoints()
{
	CurrentCheckpointIndex = 0;
	SplitTimes.Empty();

	for (AOMRCheckpoint* CP : CachedCheckpoints)
	{
		if (IsValid(CP))
		{
			CP->ResetCheckpoint();
		}
	}
}

void AOMRTimeTrialGameState::CacheCheckpoints()
{
	CachedCheckpoints.Empty();

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOMRCheckpoint::StaticClass(), Found);

	for (AActor* A : Found)
	{
		if (AOMRCheckpoint* CP = Cast<AOMRCheckpoint>(A))
		{
			CachedCheckpoints.Add(CP);
		}
	}

	TotalCheckpoints = CachedCheckpoints.Num();

	UE_LOG(LogTemp, Warning, TEXT("Cached %d checkpoints."), CachedCheckpoints.Num());
}

