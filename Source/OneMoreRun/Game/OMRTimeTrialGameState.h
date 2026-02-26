// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "OMRTimeTrialGameState.generated.h"

/**
 * 
 */
UCLASS()
class ONEMORERUN_API AOMRTimeTrialGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AOMRTimeTrialGameState();

	// Functions
	void StartLap();
	void CompleteLap();
	void HandleStartFinishCross();
		
	// Lap State
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentLap = 0;

	UPROPERTY(BlueprintReadOnly)
	float LapStartTime = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float CurrentLapTime = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float BestLapTime = -1.f;

	UPROPERTY(BlueprintReadOnly)
	bool bLapActive = false;

	UPROPERTY()
	float LastGateCrossTime = -1.f;
	
	UPROPERTY(EditDefaultsOnly)
	float GateCooldown = 2.0f;

	UFUNCTION(BlueprintPure)
	float GetDisplayedLaptime() const;

	// Checkpoint System
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentCheckpointIndex = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TotalCheckpoints= 0.f;

	UPROPERTY(BlueprintReadOnly)
	TArray<float> SplitTimes;

	UPROPERTY(BlueprintReadOnly)
	TArray<float> BestSplitTimes;

	UPROPERTY(BlueprintReadOnly)
	FTransform LastCheckpointTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TimeTrial | Checkpoints")
	bool bRequireCheckpointsToFinish = true;

	UFUNCTION(BlueprintCallable)
	void RegisterCheckpointHit(int32 CheckpointIndex, const FTransform& CheckpointTransform);

	UFUNCTION(BlueprintCallable)
	void ResetCheckpoints();

	UPROPERTY()
	TArray<TObjectPtr<class AOMRCheckpoint>> CachedCheckpoints;

	UFUNCTION()
	void CacheCheckpoints();

	// UI updates
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLapTimeUpdated, float, NewTime);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBestTimeUpdated, float, NewBestTime); 
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLapNumberUpdated, int32, NewLap);

	UPROPERTY(BlueprintAssignable)
	FOnLapTimeUpdated OnLapTimeUpdated;

	UPROPERTY(BlueprintAssignable)
	FOnBestTimeUpdated OnBestTimeUpdated;

	UPROPERTY(BlueprintAssignable)
	FOnLapNumberUpdated OnLapNumberUpdated;

	float LapTimeBroadcastAccumulator = 0.f;
	float LastBroadcastLapTime = 0.f;

	void UpdateLapTimer(float DeltaTime);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSplitUpdated, float, SplitTime, float, Splitdelta, bool, bIsAhead);

	UPROPERTY(BlueprintAssignable)
	FOnSplitUpdated OnSplitUpdated;

protected:
	virtual void BeginPlay() override;

	virtual void Tick(float DeltaTime) override;
};
