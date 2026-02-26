// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "OMRPlayerPawn.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UCameraComponent;
class USceneComponent;
class UAudioComponent;
class USoundBase;

UCLASS()
class ONEMORERUN_API AOMRPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	AOMRPlayerPawn();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;


	// Movement
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);


	// Behaviour
	void ApplyMovementForce(float DeltaTime);
	virtual bool IsGrounded() const;


	// Implementation Details
	bool GetGroundHit(FHitResult& OutHit) const;
	// Grounding
	bool UpdateGroundedState(float DeltaTime);

	FVector GetMovementInputVector(const FVector& GroundNormal) const;
	void ClampVelocity();
	float GetSlopeForceMultiplier(const FVector& GroundNormal) const;
	void SyncAngularVelocityWithLinear();
	void UpdateCamera(float DeltaTime);
	void SyncActorToPhysics();
	void StartRacePhysics();
	void UpdateMovement(float DeltaTime);
	void UpdateLandingTimers(float DeltaTime);

	// Bonus Flavour
	UFUNCTION()
		void OnHit(
			UPrimitiveComponent* HitComp,
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			FVector NormalImpulse,
			const FHitResult& Hit
		);

	UPROPERTY(EditAnywhere, Category = "Movement|Rotation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AngularVelocityBlend = 0.2f;


	// Input
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* IMC_Player;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_MoveForward;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_MoveRight;

	float MoveForwardValue = 0.f;
	float MoveRightValue = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Hop;

	bool bCanHop = true;


	// Movement
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveForce = 300000.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxSpeed = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float AirControlMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Movement|Hop")
	float HopImpulse = 400, f;

	UPROPERTY(EditAnywhere, Category = "Movement|Hop")
	float HopCooldown = 0.2f;

	float LastHopTime = -1.f;

	void Hop();

	bool bIsGrounded = false;
	FHitResult CachedGroundHit;

	// Grounding stability
	bool bGroundedStable = false;
	float GroundedCoyoteTime = 0.f;
	float GroundedConfirmTime = 0.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Grounding")
	float CoyoteTimeDuration = 0.08f; // 80ms grace

	UPROPERTY(EditAnywhere, Category = "Movement|Grounding")
	float GroundConfirmDuration = 0.03f; // 30ms confirm


	// Landing impact damp
	float PostImpactGraceTime = 0.f;
	float LandingDampTimeRemaining = 0.f;

	UPROPERTY(EditAnywhere, Category = "Movement|Feel")
	float LandingDampDuration = 0.06f; // 60ms

	UPROPERTY(EditAnywhere, Category = "Movement|Feel")
	float LandingDampMultiplier = 0.25f; // 25% force during damp

	UPROPERTY(EditAnywhere, Category = "Movement|Feel")
	float MinLandingDownSpeed = 1200.f; // only if falling fast

	UPROPERTY(EditAnywhere, Category = "Movement|Feel")
	float MinLandingImpulse = 20000.f;  // filters micro-contact spam


	// Components
	UPROPERTY(VisibleAnywhere, Category="Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* VisualMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* CameraRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* SceneRoot;   // NEW: non-physics root


	// Camera tuning
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraHeightOffset = 50.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraZInterpSpeed = 6.5f;

	float SmoothedCameraZ = 0.f;

	float CurrentFOV;

	// Speed-based camera feel
	UPROPERTY(EditAnywhere, Category = "Camera")
	float SpeedCameraMinDistance = 420.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float SpeedCameraMaxDistance = 560.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float SpeedCameraFOVMin = 90.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float SpeedCameraFOVMax = 100.f;

	FVector SmoothedMoveDir = FVector::ForwardVector;

	float SmoothedCameraSpeed = 0.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraSpeedInterp = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Camera|Framing")
	float LookAtHeight = 35.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Framing")
	float LookAheadMin = 120.f;

	UPROPERTY(EditAnywhere, Category = "Camera|Framing")
	float LookAheadMax = 320.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraPositionInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraRotationInterpSpeed = 14.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraLocalBackOffset = 420.f;

	float LandingCameraLockTime = 0.f;

	float CameraVerticalAnchorZ = 0.f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraDistanceInterpSpeed = 6.0f;

	float CurrentCameraDistance = 0.f;


	// Smoothed player input (NOT camera)
	FVector SmoothedInputDir = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Movement|Input")
	float InputDirInterpSpeed = 12.0f;


	// Airborne camera behavior
	UPROPERTY(EditAnywhere, Category = "Camera|Air")
	float AirborneFollowDelay = 0.12f; // seconds before camera follows upward

	UPROPERTY(EditAnywhere, Category = "Camera|Air")
	float AirborneZInterpSpeed = 4.5f;

	float TimeSinceUngrounded = 0.f;


	// Materials
	UPROPERTY(EditDefaultsOnly, Category = "Physics")
	UPhysicalMaterial* BallPhysicalMaterial;


	// Run Reset
	UFUNCTION(BlueprintCallable)
	void ResetRun();

	FTransform SpawnTransform;


	// Countdown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Countdown")
	float CountdownDuration = 3.0f;

	float CountdownTimeRemaining = 0.f;

	bool bCountdownActive = true;

	UPROPERTY(EditAnywhere, Category = "Countdown")
	float StartImpulseStrength = 800.f;

	UFUNCTION()
	void UpdateCountdown(float DeltaTime);

	float GoDisplayTimeRemaining = 0.f;

	UPROPERTY(EditAnywhere, Category = "Countdown")
	float GoDisplayDuration = 0.75f;

	int32 LastBroadcastCountdown = -1;

	bool bWasShowingGO = false;

	void SnapToGround();
	

	// Ball audio
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAudioComponent* RollAudio;

	UFUNCTION()
	void PlayBallAudio();

	float SmoothedPitch;
	float SmoothedVolume;
	float AirAlpha = 0.f;

	void HandleLanding();

	bool bWasGrounded = true;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* LandingSound;


	// Camera Shake
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	TSubclassOf<UCameraShakeBase> LandingShakeClass;

public:	
	virtual void Tick(float DeltaTime) override;

};
