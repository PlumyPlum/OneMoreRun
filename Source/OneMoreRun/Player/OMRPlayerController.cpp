// Fill out your copyright notice in the Description page of Project Settings.


#include "OMRPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "../UI/OMRTimeTrialHUD.h"
#include "../Game/OMRTimeTrialGameState.h"


AOMRPlayerController::AOMRPlayerController()
{
}

void AOMRPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (TimeTrialHUDClass)
    {
        TimeTrialHUD = CreateWidget<UOMRTimeTrialHUD>(this, TimeTrialHUDClass);

        if (TimeTrialHUD)
        {
            TimeTrialHUD->AddToViewport();
        }
    }

    AOMRTimeTrialGameState* GS = GetWorld()->GetGameState<AOMRTimeTrialGameState>();
    if (GS)
    {
        GS->OnLapTimeUpdated.AddDynamic(this, &AOMRPlayerController::HandleLapTimeUpdated);
        GS->OnBestTimeUpdated.AddDynamic(this, &AOMRPlayerController::HandleBestTimeUpdated);
        GS->OnLapNumberUpdated.AddDynamic(this, &AOMRPlayerController::HandleLapNumberUpdated);
        GS->OnSplitUpdated.AddDynamic(this, &AOMRPlayerController::HandleSplitUpdated);
    }
}

void AOMRPlayerController::OnCountdownChanged(int32 NewValue)
{
    if (TimeTrialHUD)
    {
        TimeTrialHUD->UpdateCountdownText(NewValue);
    }
}

void AOMRPlayerController::OnCountdownGo()
{

    if (TimeTrialHUD)
    {

        TimeTrialHUD->PlayGoAnimation();
    }

}

void AOMRPlayerController::HandleLapTimeUpdated(float NewTime)
{
    if (TimeTrialHUD)
    {
        TimeTrialHUD->UpdateLapTime(NewTime);
    }
}

void AOMRPlayerController::HandleBestTimeUpdated(float NewBestTime)
{
    if (TimeTrialHUD)
    {
        TimeTrialHUD->UpdateBestTime(NewBestTime);
    }
}

void AOMRPlayerController::HandleLapNumberUpdated(int32 NewLap)
{
    if (TimeTrialHUD)
    {
        TimeTrialHUD->UpdateLapNumber(NewLap);
    }
}

void AOMRPlayerController::HandleSplitUpdated(float SplitTime, float SplitDelta, bool bIsAhead)
{
    if (TimeTrialHUD)
    {
        TimeTrialHUD->UpdateSplit(SplitTime, SplitDelta, bIsAhead);
    }
}


