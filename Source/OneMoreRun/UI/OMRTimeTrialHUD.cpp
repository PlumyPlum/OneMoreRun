// Fill out your copyright notice in the Description page of Project Settings.


#include "../UI/OMRTimeTrialHUD.h"

FText UOMRTimeTrialHUD::FormatTime(float Time) const
{
	int32 Minutes = FMath::FloorToInt(Time / 60.f);
	int32 Seconds = FMath::FloorToInt(Time) % 60;
	int32 Centiseconds = FMath::FloorToInt((Time - FMath::FloorToInt(Time)) * 100.f);
	return FText::FromString(
		FString::Printf(TEXT("%d:%02d:%02d"), Minutes, Seconds, Centiseconds)
	);
}
