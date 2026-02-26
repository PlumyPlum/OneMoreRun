// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OMRCheckpoint.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class ONEMORERUN_API AOMRCheckpoint : public AActor
{
	GENERATED_BODY()
	
public:	
	AOMRCheckpoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Checkpoint")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	TObjectPtr<UBoxComponent> Trigger;

	// Set this per placed checkpoint in the level:  0, 1, 2, 3
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Checkpoint")
	int32 CheckpointIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void ResetCheckpoint();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
};
