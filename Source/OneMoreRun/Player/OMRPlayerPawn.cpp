// Fill out your copyright notice in the Description page of Project Settings.


#include "OMRPlayerPawn.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "OMRPlayerController.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

AOMRPlayerPawn::AOMRPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Root (non-physics)
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Physics sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(SceneRoot); // ✅ FIX

	CollisionSphere->InitSphereRadius(50.f);
	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetEnableGravity(true);
	CollisionSphere->SetCollisionProfileName(TEXT("Pawn"));
	CollisionSphere->SetLinearDamping(0.15f);
	CollisionSphere->SetAngularDamping(0.05f);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->OnComponentHit.AddDynamic(this, &AOMRPlayerPawn::OnHit);
	CollisionSphere->SetUseCCD(true);

	// Visual mesh (must follow physics rotation)
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionSphere); // ✅ FIX
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Camera rig (NOT parented to physics)
	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	CameraRoot->SetupAttachment(SceneRoot); // ✅ FIX

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraRoot);

	// Ball Audio
	RollAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RollAudio"));
	RollAudio->SetupAttachment(SceneRoot);
	RollAudio->bAutoActivate = true;
}

void AOMRPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (!PC->IsLocalController()) return;

		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Subsystem->AddMappingContext(IMC_Player, 0);
			}
		}
	}

	if (BallPhysicalMaterial)
	{
		CollisionSphere->SetPhysMaterialOverride(BallPhysicalMaterial);
	}

	SnapToGround();

	CollisionSphere->SetSimulatePhysics(false);
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

	CountdownTimeRemaining = CountdownDuration;
	bCountdownActive = true;

	SpawnTransform = GetActorTransform();
	CameraVerticalAnchorZ = CollisionSphere->GetComponentLocation().Z;
	SmoothedCameraZ = CameraVerticalAnchorZ + CameraHeightOffset;
	
	SmoothedMoveDir =
		FVector::VectorPlaneProject(
			SpawnTransform.GetRotation().GetForwardVector(),
			FVector::UpVector
		).GetSafeNormal();

	CurrentCameraDistance = SpeedCameraMinDistance;

	CurrentFOV = Camera ? Camera->FieldOfView : SpeedCameraFOVMin;
}

void AOMRPlayerPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SyncActorToPhysics();

	UpdateCountdown(DeltaTime);

	UpdateGroundedState(DeltaTime);

	if(!bWasGrounded && bIsGrounded)
	{
		bCanHop = true;
	}

	bWasGrounded = bIsGrounded;
	UpdateCamera(DeltaTime);

	UpdateMovement(DeltaTime);

	UpdateLandingTimers(DeltaTime);

	PlayBallAudio();
	HandleLanding();
}

void AOMRPlayerPawn::ResetRun()
{
	if (!CollisionSphere) return;

	// -------------------------------------------------
	// 1. Stop physics completely
	// -------------------------------------------------
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

	// -------------------------------------------------
	// 2. Teleport physics body (authoritative)
	// -------------------------------------------------
	CollisionSphere->SetWorldTransform(
		SpawnTransform,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);

	// Keep actor/root aligned
	SetActorTransform(SpawnTransform);

	// -------------------------------------------------
	// 3. Reset camera vertical state
	// -------------------------------------------------
	CameraVerticalAnchorZ = SpawnTransform.GetLocation().Z;
	SmoothedCameraZ = CameraVerticalAnchorZ + CameraHeightOffset;

	// -------------------------------------------------
	// 4. Reset camera direction & input smoothing
	// -------------------------------------------------
	if (Camera)
	{
		const FVector Forward =
			FVector::VectorPlaneProject(
				SpawnTransform.GetRotation().GetForwardVector(),
				FVector::UpVector
			).GetSafeNormal();

		SmoothedMoveDir = Forward;
	}
	else
	{
		SmoothedMoveDir = FVector::ForwardVector;
	}

	SmoothedCameraSpeed = 0.f;

	// -------------------------------------------------
	// 5. Clear transient gameplay state
	// -------------------------------------------------
	LandingDampTimeRemaining = 0.f;
	LandingCameraLockTime = 0.f;
	TimeSinceUngrounded = 0.f;
}

void AOMRPlayerPawn::UpdateCountdown(float DeltaTime)
{
	if (!bCountdownActive)
		return;

	CountdownTimeRemaining -= DeltaTime;

	if (CountdownTimeRemaining <= 0.f)
	{
		bCountdownActive = false;

		if (AOMRPlayerController* PC = Cast<AOMRPlayerController>(GetController()))
		{
			PC->OnCountdownGo();
		}

		StartRacePhysics();
		return;
	}

	int32 CurrentCount = FMath::CeilToInt(CountdownTimeRemaining);

	if (CurrentCount != LastBroadcastCountdown)
	{
		LastBroadcastCountdown = CurrentCount;

		if (AOMRPlayerController* PC = Cast<AOMRPlayerController>(GetController()))
		{
			PC->OnCountdownChanged(CurrentCount);
		}
	}
}

void AOMRPlayerPawn::SnapToGround()
{
	if (!CollisionSphere) return;

	FVector Start = CollisionSphere->GetComponentLocation();
	FVector End = Start - FVector(0.f, 0.f, 500.f); // trace downward

	FHitResult Hit;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_WorldStatic,
		Params
	);

	if (bHit)
	{
		float Radius = CollisionSphere->GetScaledSphereRadius();

		FVector NewLocation = Hit.Location + FVector(0.f, 0.f, Radius);

		CollisionSphere->SetWorldLocation(NewLocation);
		SetActorLocation(NewLocation);
	}
}

void AOMRPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent)) 
	{
		EIC->BindAction(IA_MoveForward, ETriggerEvent::Triggered, this, &AOMRPlayerPawn::MoveForward);
		EIC->BindAction(IA_MoveForward, ETriggerEvent::Completed, this, &AOMRPlayerPawn::MoveForward);

		EIC->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &AOMRPlayerPawn::MoveRight);
		EIC->BindAction(IA_MoveRight, ETriggerEvent::Completed, this, &AOMRPlayerPawn::MoveRight);

		EIC->BindAction(IA_Hop, ETriggerEvent::Started, this, &AOMRPlayerPawn::Hop);
	}

}

void AOMRPlayerPawn::MoveForward(const FInputActionValue& Value)
{
	if (bCountdownActive) return;
	MoveForwardValue = Value.Get<float>();
}

void AOMRPlayerPawn::MoveRight(const FInputActionValue& Value)
{
	if (bCountdownActive) return;
	MoveRightValue = Value.Get<float>();
}

bool AOMRPlayerPawn::IsGrounded() const
{
	FHitResult Hit;
	return GetGroundHit(Hit);
}

bool AOMRPlayerPawn::GetGroundHit(FHitResult& OutHit) const
{
	const FVector Start = CollisionSphere->GetComponentLocation();

	// Sweep down a bit further than your old trace
	const float SweepDistance = 70.f;

	// Slightly smaller than actual radius to avoid snagging edges
	const float Radius = CollisionSphere->GetScaledSphereRadius() * 0.95f;

	const FVector End = Start - FVector(0.f, 0.f, SweepDistance);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		ECC_WorldStatic, // IMPORTANT: don't use Visibility here
		Shape,
		Params
	);

	if (!bHit) return false;

	// Filter: only accept walkable-ish surfaces
	return OutHit.Normal.Z > 0.55f;
}

bool AOMRPlayerPawn::UpdateGroundedState(float DeltaTime)
{
	FHitResult Hit;
	// -------------------------------------------------
	// 1. Raw grounding check (sphere sweep)
	// -------------------------------------------------
	const bool bRawGrounded = GetGroundHit(Hit);

	// -------------------------------------------------
	// 2. Stabilize grounded state (confirm + coyote)
	// -------------------------------------------------
	if (bRawGrounded)
	{
		CachedGroundHit = Hit;

		// Build confidence that we are grounded
		GroundedConfirmTime += DeltaTime;

		// Reset coyote timer every grounded frame
		GroundedCoyoteTime = CoyoteTimeDuration;

		// Only switch to grounded after confirm duration
		if (!bGroundedStable && GroundedConfirmTime >= GroundConfirmDuration)
		{
			bGroundedStable = true;
		}
	}
	else
	{
		// Lost raw ground contact
		GroundedConfirmTime = 0.f;

		// Count down coyote time
		GroundedCoyoteTime = FMath::Max(
			GroundedCoyoteTime - DeltaTime,
			0.f
		);

		// Only unground after coyote expires
		if (bGroundedStable && GroundedCoyoteTime <= 0.f)
		{
			bGroundedStable = false;
		}
	}
	bIsGrounded = bGroundedStable;

	return bIsGrounded;

}

FVector AOMRPlayerPawn::GetMovementInputVector(const FVector& GroundNormal) const
{
	if (!Camera)
	{
		return FVector::ZeroVector;
	}

	// Camera basis
	FVector CamForward = Camera->GetForwardVector();
	FVector CamRight = Camera->GetRightVector();

	// Flatten onto ground plane
	CamForward = FVector::VectorPlaneProject(CamForward, GroundNormal).GetSafeNormal();
	CamRight = FVector::VectorPlaneProject(CamRight, GroundNormal).GetSafeNormal();

	FVector InputDir =
		(CamForward * MoveForwardValue) +
		(CamRight * MoveRightValue);

	return InputDir.IsNearlyZero() ? FVector::ZeroVector : InputDir.GetSafeNormal();
}

void AOMRPlayerPawn::ApplyMovementForce(float DeltaTime)
{
	if (!CollisionSphere) return;

	FVector GroundNormal = FVector::UpVector;

	if (bIsGrounded && CachedGroundHit.IsValidBlockingHit())
	{
		GroundNormal = CachedGroundHit.ImpactNormal;
	}
	GroundNormal = GroundNormal.GetSafeNormal();

	FVector InputDir = GetMovementInputVector(GroundNormal);

	// Smooth INPUT intent only (not camera direction)
	SmoothedInputDir = FMath::VInterpTo(
		SmoothedInputDir,
		InputDir,
		GetWorld()->GetDeltaSeconds(),
		InputDirInterpSpeed
	);

	// Kill micro drift
	if (SmoothedInputDir.SizeSquared() < 0.001f)
	{
		SmoothedInputDir = FVector::ZeroVector;
	}

	InputDir = SmoothedInputDir;

	if (InputDir.IsZero()) return;

	const FVector Velocity = CollisionSphere->GetPhysicsLinearVelocity();
	const float Speed = Velocity.Size();

	// Speed-based ramp (prevents snap accel)
	const float SpeedAlpha = FMath::Clamp(Speed / MaxSpeed, 0.f, 1.f);
	if (Speed >= MaxSpeed && FVector::DotProduct(Velocity, InputDir) > 0.f) { return; }
	const float ForceScale = FMath::Lerp(0.35f, 1.0f, SpeedAlpha);
	const float SteeringReduction = 1.f - SpeedAlpha * 0.4f;
	InputDir *= SteeringReduction;

	// Landing damp (temporary force reduction)
	float LandingDamp = 1.0f;
	if (LandingDampTimeRemaining > 0.f)
	{
		LandingDamp = LandingDampMultiplier; // e.g. 0.25
	}

	float ControlMultiplier = bIsGrounded ? 1.0f : AirControlMultiplier;

	if (bIsGrounded)
	{
		// Project input onto the ground plane
		InputDir = FVector::VectorPlaneProject(InputDir, GroundNormal).GetSafeNormal();

		if (InputDir.IsNearlyZero())
		{
			return;
		}

		const float DownhillFactor =
			FVector::DotProduct(GroundNormal, FVector::UpVector);

		const float SlopeBoost =
			FMath::Lerp(1.1f, 1.0f, DownhillFactor);

		const float SlopeMultiplier =
			GetSlopeForceMultiplier(GroundNormal) * SlopeBoost;
		CollisionSphere->AddForce(
			InputDir *
			MoveForce *
			SlopeMultiplier *
			ForceScale *
			LandingDamp
		);

		return; // grounded force applied
	}

	// Air / fallback
	CollisionSphere->AddForce(
		InputDir * MoveForce * ControlMultiplier *
		ForceScale * LandingDamp
	);
}

void AOMRPlayerPawn::ClampVelocity()
{
	if (!CollisionSphere) return;

	FVector Velocity = CollisionSphere->GetPhysicsLinearVelocity();


	// Separate horizontal and vertical velocity
	FVector HorizontalVel(Velocity.X, Velocity.Y, 0.f);
	const float HorizontalSpeed = HorizontalVel.Size();

	if (HorizontalSpeed > MaxSpeed)
	{
		HorizontalVel = HorizontalVel.GetSafeNormal() * MaxSpeed;

		// Recombine with ORIGINAL vertical velocity
		Velocity.X = HorizontalVel.X;
		Velocity.Y = HorizontalVel.Y;

		CollisionSphere->SetPhysicsLinearVelocity(Velocity);
	}
}

float AOMRPlayerPawn::GetSlopeForceMultiplier(const FVector& GroundNormal) const
{
	// Dot of Ground Normal vs World Up
	const float SlopeDot = FVector::DotProduct(GroundNormal, FVector::UpVector);

	// convert to slope steepness (0 =  flat, 1 = vertical)
	const float Steepness = 1.0f - SlopeDot;

	// Designer tunable curve
	// 0.0 = full force, 1.0 = no force
	const float MinSlopeDot = 0.65f;
	const float MaxSlopeDot = 0.95f;

	return FMath::Clamp(
		FMath::GetMappedRangeValueClamped(
			FVector2D(MaxSlopeDot, MinSlopeDot),
			FVector2D(1.0f, 0.1f),
			SlopeDot
		),
		0.1f,
		1.0f
	);
}

void AOMRPlayerPawn::SyncAngularVelocityWithLinear()
{
	if (!CollisionSphere) return;

	if (!bIsGrounded) return;
	
	FVector GroundNormal = FVector::UpVector;

	if (CachedGroundHit.IsValidBlockingHit())
	{
		GroundNormal = CachedGroundHit.ImpactNormal.GetSafeNormal();
	}
	const FVector LinearVelocity =
		CollisionSphere->GetPhysicsLinearVelocity();

	// Ignore tiny motion
	if (LinearVelocity.SizeSquared() < 10.f)
	{
		return;
	}

	const float Radius = CollisionSphere->GetScaledSphereRadius();

	// Direction of travel along surface
	const FVector VelocityDir = LinearVelocity.GetSafeNormal();

	// Rolling axis = perpendicular to velocity AND surface normal
	FVector RotationAxis =
		FVector::CrossProduct(VelocityDir, GroundNormal);

	if (RotationAxis.IsNearlyZero())
	{
		return;
	}

	RotationAxis.Normalize();

	// ω = v / r
	const float AngularSpeed = LinearVelocity.Size() / Radius;

	const FVector TargetAngularVelocity =
		RotationAxis * AngularSpeed;

	const FVector CurrentAngularVelocity =
		CollisionSphere->GetPhysicsAngularVelocityInRadians();

	const FVector NewAngularVelocity =
		FMath::VInterpTo(
			CurrentAngularVelocity,
			TargetAngularVelocity,
			GetWorld()->GetDeltaSeconds(),
			15.f
		);

	CollisionSphere->SetPhysicsAngularVelocityInRadians(
		NewAngularVelocity,
		false // DO NOT add to existing
	);
}

void AOMRPlayerPawn::UpdateCamera(float DeltaTime)
{
	if (!CollisionSphere || !CameraRoot || !Camera) return;

	if (bIsGrounded)
	{
		TimeSinceUngrounded = 0.f;
	}
	else
	{
		TimeSinceUngrounded += DeltaTime;
	}

	if (LandingCameraLockTime > 0.f)
	{
		LandingCameraLockTime -= DeltaTime;
	}

	Camera->SetFieldOfView(CurrentFOV);

	// -------------------------------------------------
	// 1) Gather physics state
	// -------------------------------------------------
	const FVector PhysicsLoc = CollisionSphere->GetComponentLocation();

	FVector GroundNormal = FVector::UpVector;

	if (bIsGrounded && CachedGroundHit.IsValidBlockingHit())
	{
		GroundNormal = CachedGroundHit.ImpactNormal.GetSafeNormal();
	}

	const FVector Velocity = CollisionSphere->GetPhysicsLinearVelocity();
	const FVector HorizontalVel(Velocity.X, Velocity.Y, 0.f);
	const float RawSpeed = HorizontalVel.Size();

	// -------------------------------------------------
	// 2) Update smoothed move direction (ONCE)
	//    - Grounded: follow velocity direction
	//    - Airborne: keep last stable direction (prevents flipping)
	//    - Landing lock: freeze direction briefly on heavy impacts
	// -------------------------------------------------
	if (bIsGrounded && LandingCameraLockTime <= 0.f && !HorizontalVel.IsNearlyZero())
	{
		const FVector DesiredDir = HorizontalVel.GetSafeNormal();
		SmoothedMoveDir = FMath::VInterpTo(
			SmoothedMoveDir,
			DesiredDir,
			DeltaTime,
			10.0f
		);
	}

	// Safety: if we ever lose a valid direction, default forward-ish
	if (SmoothedMoveDir.IsNearlyZero() && !HorizontalVel.IsNearlyZero())
	{
		SmoothedMoveDir = HorizontalVel.GetSafeNormal();
	}

	// -------------------------------------------------
	// 3) Smooth speed (for stable alpha)
	// -------------------------------------------------
	SmoothedCameraSpeed = FMath::FInterpTo(
		SmoothedCameraSpeed,
		RawSpeed,
		DeltaTime,
		CameraSpeedInterp
	);

	// -------------------------------------------------
	// 4) Speed alpha shaping (the "juice")
	//    - Linear alpha feels flat unless you hit MaxSpeed
	//    - Pow(<1) makes camera react earlier (more fun)
	// -------------------------------------------------
	const float SpeedNormalized = (MaxSpeed > KINDA_SMALL_NUMBER)
		? (SmoothedCameraSpeed / MaxSpeed)
		: 0.f;

	const float SpeedAlpha = FMath::Clamp(
		FMath::Pow(FMath::Clamp(SpeedNormalized, 0.f, 1.f), 0.5f),
		0.f,
		1.f
	);

	// -------------------------------------------------
	// 5) Vertical anchoring / airborne follow (your system, cleaned)
	// -------------------------------------------------
	if (bIsGrounded)
	{
		// Hard anchor to ground when grounded
		CameraVerticalAnchorZ = PhysicsLoc.Z;
	}
	else
	{
		// If airborne long enough, let camera follow upward
		if (TimeSinceUngrounded > AirborneFollowDelay)
		{
			CameraVerticalAnchorZ = FMath::FInterpTo(
				CameraVerticalAnchorZ,
				PhysicsLoc.Z,
				DeltaTime,
				AirborneZInterpSpeed
			);
		}
	}

	// Prevent camera lagging too far below the ball
	const float MaxVerticalLag = 180.f;
	CameraVerticalAnchorZ = FMath::Max(CameraVerticalAnchorZ, PhysicsLoc.Z - MaxVerticalLag);

	// Target Z = anchor + offset
	const float DesiredZ = CameraVerticalAnchorZ + CameraHeightOffset;

	SmoothedCameraZ = FMath::FInterpTo(
		SmoothedCameraZ,
		DesiredZ,
		DeltaTime,
		CameraZInterpSpeed
	);

	// Clamp camera Z relative to ball (keeps framing stable)
	const float MaxBelow = 180.f;
	const float MaxAbove = 360.f;

	SmoothedCameraZ = FMath::Clamp(
		SmoothedCameraZ,
		PhysicsLoc.Z - MaxBelow,
		PhysicsLoc.Z + MaxAbove
	);

	// -------------------------------------------------
	// 6) Speed-based pullback (WITH distance smoothing)
	// -------------------------------------------------
	const float TargetDistance = FMath::Lerp(
		SpeedCameraMinDistance,
		SpeedCameraMaxDistance,
		SpeedAlpha
	);

	CurrentCameraDistance = FMath::FInterpTo(
		CurrentCameraDistance,
		TargetDistance,
		DeltaTime,
		CameraDistanceInterpSpeed
	);

	const FVector BackDir = -SmoothedMoveDir;

	const FVector DesiredCameraLoc =
		FVector(PhysicsLoc.X, PhysicsLoc.Y, SmoothedCameraZ) +
		(BackDir * CurrentCameraDistance);

	CameraRoot->SetWorldLocation(
		FMath::VInterpTo(
			CameraRoot->GetComponentLocation(),
			DesiredCameraLoc,
			DeltaTime,
			CameraPositionInterpSpeed
		)
	);

	// -------------------------------------------------
	// 7) Look-ahead framing (shaped so low-speed stays stable)
	// -------------------------------------------------
	const float LookAheadAlpha = FMath::Pow(SpeedAlpha, 0.8f);

	const float LookAhead = FMath::Lerp(
		LookAheadMin,
		LookAheadMax,
		LookAheadAlpha
	);

	const FVector LookTarget =
		PhysicsLoc +
		(SmoothedMoveDir * LookAhead) +
		FVector(0.f, 0.f, LookAtHeight);;

	const FVector CamLoc = Camera->GetComponentLocation();

	const FRotator TargetRot = (LookTarget - CamLoc).Rotation();

	const FRotator NewRot = FMath::RInterpTo(
		Camera->GetComponentRotation(),
		TargetRot,
		DeltaTime,
		CameraRotationInterpSpeed > 0.f ? CameraRotationInterpSpeed : CameraPositionInterpSpeed
	);

	// -------------------------------------------------
	// 8) Optional: subtle banking (feels fast when carving)
	//     - Only if we have meaningful horizontal movement
	// -------------------------------------------------
	FRotator FinalRot = NewRot;

	if (HorizontalVel.SizeSquared() > 25.f)
	{
		const FVector VelDir = HorizontalVel.GetSafeNormal();

		// + = turning right, - = turning left relative to camera forward
		const float TurnAmount = FVector::DotProduct(
			FVector::CrossProduct(SmoothedMoveDir, VelDir),
			FVector::UpVector
		);

		const float MaxBankDeg = 5.0f;
		const float TargetRoll = TurnAmount * MaxBankDeg;

		FinalRot.Roll = FMath::FInterpTo(
			Camera->GetComponentRotation().Roll,
			TargetRoll,
			DeltaTime,
			5.0f
		);
	}
	else
	{
		// Return roll to neutral if stopped
		FinalRot.Roll = FMath::FInterpTo(
			Camera->GetComponentRotation().Roll,
			0.f,
			DeltaTime,
			5.0f
		);
	}

	/*float PitchBoost = FMath::Lerp(0.f, -4.f, SpeedAlpha);
	FinalRot.Pitch += PitchBoost;*/

	Camera->SetWorldRotation(FinalRot);

	// -------------------------------------------------
	// 9) Speed-based FOV (WITH its own smoothing)
	// -------------------------------------------------
		const float TargetFOV = FMath::Lerp(
			SpeedCameraFOVMin,
			SpeedCameraFOVMax,
			SpeedAlpha
		);

		const float FOVInterpSpeed = 8.0f;

		// Smooth our own tracked value (prevents fighting external writes)
		CurrentFOV = FMath::FInterpTo(
			CurrentFOV,
			TargetFOV,
			DeltaTime,
			FOVInterpSpeed
		);

		Camera->SetFieldOfView(CurrentFOV);
}

void AOMRPlayerPawn::SyncActorToPhysics()
{
	if (!CollisionSphere) return;
	SetActorLocation(CollisionSphere->GetComponentLocation());
}

void AOMRPlayerPawn::StartRacePhysics()
{
	// unfreeze physics
	CollisionSphere->SetSimulatePhysics(true);

	// clear any accidentally stored values
	CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CollisionSphere->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

	// small forward launch impulse
	CollisionSphere->AddImpulse(
		SpawnTransform.GetRotation().GetForwardVector() * StartImpulseStrength,
		NAME_None,
		true
	);
}

void AOMRPlayerPawn::UpdateMovement(float DeltaTime)
{
	ApplyMovementForce(DeltaTime);

	ClampVelocity();

	SyncAngularVelocityWithLinear();
}

void AOMRPlayerPawn::UpdateLandingTimers(float DeltaTime)
{
	LandingDampTimeRemaining = FMath::Max(LandingDampTimeRemaining - DeltaTime, 0.f);
	LandingCameraLockTime = FMath::Max(LandingCameraLockTime - DeltaTime, 0.f);
}

void AOMRPlayerPawn::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!CollisionSphere) return;

	const FVector Velocity = CollisionSphere->GetPhysicsLinearVelocity();

	// ---- HIGH-SPEED GROUND BOUNCE ASSIST ----
	const bool bGroundHit = (Hit.Normal.Z > 0.7f);
	const bool bFastDown = (Velocity.Z < -1800.f);

	if (FMath::Abs(Velocity.Z) < 600.f)
	{
		return; // too small to care about
	}

	if (bGroundHit && bFastDown)
	{
		if (Hit.Normal.Z > 0.7f)
		{
			LandingCameraLockTime = 0.12f; // ~7 frames at 60fps
		}

		const float Speed = Velocity.Size();
		const float AssistStrength = 0.04f; // subtle

		const FVector UpAssist =
			FVector::UpVector * Speed * AssistStrength;

		CollisionSphere->AddImpulse(UpAssist, NAME_None, true);
	}

	// ---- LANDING DAMP (not grace lockout) ----
	const bool bBigImpact = (NormalImpulse.Size() > 20000.f);
	if (bGroundHit && bFastDown && bBigImpact)
	{
		LandingDampTimeRemaining = LandingDampDuration;
	}
}

void AOMRPlayerPawn::Hop()
{
	if (!CollisionSphere) return;

	if (!bCanHop) return;

	if (!bIsGrounded) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime - LastHopTime < HopCooldown)
		return;

	bCanHop = false;
	LastHopTime = CurrentTime;

	FVector Velocity = CollisionSphere->GetPhysicsLinearVelocity();

	// kill downward velocity only
	if (Velocity.Z < 0.f)
	{
		Velocity.Z = 0.f;
		CollisionSphere->SetPhysicsLinearVelocity(Velocity);
	}

	CollisionSphere->AddImpulse(
		FVector::UpVector * HopImpulse,
		NAME_None,
		true
	);
}

void AOMRPlayerPawn::PlayBallAudio()
{
	if (!RollAudio || MaxSpeed <= 0.f) return;

	const float DeltaTime = GetWorld()->GetDeltaSeconds();

	float Speed = CollisionSphere->GetPhysicsLinearVelocity().Size();
	float NormalizedSpeed = FMath::Clamp(Speed / MaxSpeed, 0.f, 1.f);

	float SpeedAlpha = FMath::Pow(NormalizedSpeed, 1.3f);

	float BasePitch = FMath::Lerp(0.85f, 2.0f, SpeedAlpha);
	float BaseVolume = FMath::Lerp(0.4f, 1.0f, SpeedAlpha);

	float TargetAirAlpha = bIsGrounded ? 0.f : 1.f;

	AirAlpha = FMath::FInterpTo(AirAlpha, TargetAirAlpha, DeltaTime, 6.f);

	float AirPitchMultiplier = FMath::Lerp(1.0f, 1.1f, AirAlpha);
	float AirVolumeMultiplier = FMath::Lerp(1.0f, 0.75f, AirAlpha);

	float TargetPitch = BasePitch * AirPitchMultiplier;
	float TargetVolume = BaseVolume * AirVolumeMultiplier;

	SmoothedPitch = FMath::FInterpTo(SmoothedPitch, TargetPitch, DeltaTime, 8.f);
	SmoothedVolume = FMath::FInterpTo(SmoothedVolume, TargetVolume, DeltaTime, 8.f);

	RollAudio->SetPitchMultiplier(SmoothedPitch);
	RollAudio->SetVolumeMultiplier(SmoothedVolume);
}

void AOMRPlayerPawn::HandleLanding()
{
	if (!bWasGrounded && bIsGrounded)
	{
		const FVector Velocity = CollisionSphere->GetPhysicsLinearVelocity();
		const float VerticalSpeed = FMath::Abs(Velocity.Z);

		if (VerticalSpeed > 300.f)
		{
			float ImpactAlpha = FMath::Clamp(VerticalSpeed / 2000.f, 0.f, 1.f);

			float Volume = FMath::Lerp(0.3f, 1.0f, ImpactAlpha);
			float Pitch = FMath::Lerp(0.9f, 1.2f, ImpactAlpha);

			UGameplayStatics::PlaySoundAtLocation(
				this,
				LandingSound,
				GetActorLocation(),
				Volume,
				Pitch
			);
			
			float ShakeClass = ImpactAlpha;
			
			APlayerController* PC = Cast<APlayerController>(GetController());
			if (PC && LandingShakeClass)
			{
				PC->ClientStartCameraShake(LandingShakeClass, ShakeClass);
			}
		}
	}

	bWasGrounded = bIsGrounded;
}
