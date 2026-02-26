// Fill out your copyright notice in the Description page of Project Settings.

#include "OMRTimeTrialGameMode.h"
#include "OMRTimeTrialGameState.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

AOMRTimeTrialGameMode::AOMRTimeTrialGameMode()
{
	GameStateClass = AOMRTimeTrialGameState::StaticClass();
}

void AOMRTimeTrialGameMode::BeginPlay()
{
	Super::BeginPlay();

	PlayNextTrack();
}

int32 AOMRTimeTrialGameMode::GetRandomTrackIndex()
{
	if (LevelMusicTracks.Num() == 0) return -1;

	int32 NewIndex;

	do
	{
		NewIndex = FMath::RandRange(0, LevelMusicTracks.Num() - 1);
	} 
	while (NewIndex == LastTrackIndex && LevelMusicTracks.Num() > 1);

	LastTrackIndex = NewIndex;
	return NewIndex;;
}

void AOMRTimeTrialGameMode::PlayNextTrack()
{
	int32 Index = GetRandomTrackIndex();
	if (Index < 0) return;

	USoundBase* NewTrack = LevelMusicTracks[Index];
	if (!NewTrack) return;

	const float CrossfadeDuration = 10.0f;

	UAudioComponent* NewComponent = UGameplayStatics::SpawnSound2D(this, NewTrack, 1.f, 1.f, 0.f, nullptr, false);

	if (!NewComponent) return;

	// first track
	if (!CurrentMusicComponent)
	{
		NewComponent->SetVolumeMultiplier(1.0f);
	}
	else
	{
		NewComponent->FadeIn(CrossfadeDuration, 1.0f);
		CurrentMusicComponent->FadeOut(CrossfadeDuration, 0.f); // 2 second fade out
	}

	CurrentMusicComponent = NewComponent;

	const float TrackDuration = NewTrack->GetDuration();

	if (TrackDuration > CrossfadeDuration)
	{
		float TimeUntilNextFade = TrackDuration - CrossfadeDuration;

		GetWorld()->GetTimerManager().SetTimer(
			MusicTimerHandle,
			this,
			&AOMRTimeTrialGameMode::PlayNextTrack,
			TimeUntilNextFade,
			false
		);
	}
}

