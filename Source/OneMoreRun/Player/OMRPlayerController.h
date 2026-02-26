// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OMRPlayerController.generated.h"

class UOMRTimeTrialHUD;

/**
 * 
 */
UCLASS()
class ONEMORERUN_API AOMRPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AOMRPlayerController();

	UFUNCTION(BlueprintCallable)
	void OnCountdownChanged(int32 NewValue);

	UFUNCTION(BlueprintCallable)
	void OnCountdownGo();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UOMRTimeTrialHUD> TimeTrialHUDClass;

	UPROPERTY()
	UOMRTimeTrialHUD* TimeTrialHUD;

	UFUNCTION()
	void HandleLapTimeUpdated(float NewTime);

	UFUNCTION()
	void HandleBestTimeUpdated(float NewBestTime);

	UFUNCTION()
	void HandleLapNumberUpdated(int32 NewLap);

	UFUNCTION()
	void HandleSplitUpdated(float SplitTime, float SplitDelta, bool bIsAhead);
};
