// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OMRGameMode.h"
#include "OMRTimeTrialGameMode.generated.h"

class USoundBase;
class UAudioComponent;
class UUserWidget;

/**
 * 
 */
UCLASS()
class ONEMORERUN_API AOMRTimeTrialGameMode : public AOMRGameMode
{
	GENERATED_BODY()

public:
	AOMRTimeTrialGameMode();

	UFUNCTION()
	int32 GetRandomTrackIndex();

	UFUNCTION()
	void PlayNextTrack();

	UPROPERTY(EditAnywhere, Category = "Audio")
	TArray<USoundBase*> LevelMusicTracks;

	UPROPERTY()
	UAudioComponent* CurrentMusicComponent;

	int32 LastTrackIndex = -1;

	FTimerHandle MusicTimerHandle;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> TimeTrialHUDClass;


};
