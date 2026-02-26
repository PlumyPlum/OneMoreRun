// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OMRTimeTrialHUD.generated.h"

/**
 * 
 */
UCLASS()
class ONEMORERUN_API UOMRTimeTrialHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:

    UFUNCTION(BlueprintImplementableEvent)
    void UpdateLapTime(float NewTime);

    UFUNCTION(BlueprintImplementableEvent)
    void UpdateBestTime(float NewBestTime);

    UFUNCTION(BlueprintImplementableEvent)
    void UpdateLapNumber(int32 NewLap);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void UpdateCountdownText(int32 NewValue);

    UFUNCTION(BlueprintImplementableEvent, Category = "UI")
    void PlayGoAnimation();

    UFUNCTION(BlueprintCallable)
    FText FormatTime(float Time) const;

    UFUNCTION(BlueprintImplementableEvent)
    void UpdateSplit(float SplitTime, float SplitDelta, bool bIsAhead);
};
