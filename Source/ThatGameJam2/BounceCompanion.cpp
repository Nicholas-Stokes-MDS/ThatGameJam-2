// CrawlerCompanion.cpp

#include "BounceCompanion.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

ABounceCompanion::ABounceCompanion()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABounceCompanion::BeginPlay()
{
	Super::BeginPlay();

	BaseRadius = Collision->GetUnscaledSphereRadius();
	CurrentScale = 1.f; // Never starts expanded; only a pulse expands it
}

void ABounceCompanion::ApplyIdleCollision()
{
	Super::ApplyIdleCollision();

	// Sweeps while crawling stop at level geometry.
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
}

void ABounceCompanion::OnStateChanged(ECompanionState OldState, ECompanionState NewState)
{
	// Picked up or thrown mid-pulse: cancel it
	if (NewState != ECompanionState::Idle)
	{
		PulseTimeRemaining = 0.f;
	}
}

void ABounceCompanion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PulseTimeRemaining = FMath::Max(0.f, PulseTimeRemaining - DeltaSeconds);
	const bool bPulsing = PulseTimeRemaining > 0.f;

	// Expanded only during a pulse, otherwise normal size
	const float TargetScale = bPulsing ? ExpandedScale : 1.f;
	const float InterpSpeed = bPulsing ? ExpandInterpSpeed : ContractInterpSpeed;
	CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, DeltaSeconds, InterpSpeed);
	Mesh->SetRelativeScale3D(FVector(CurrentScale));

	switch (GetCompanionState())
	{
	case ECompanionState::Idle:
		Collision->SetSphereRadius(BaseRadius * CurrentScale);
		UpdateCrawl(DeltaSeconds);
		if (bPulsing)
		{
			ApplyPush(DeltaSeconds);
		}
		break;

	case ECompanionState::Held:
		if (!FMath::IsNearlyEqual(Collision->GetUnscaledSphereRadius(), BaseRadius))
		{
			Collision->SetSphereRadius(BaseRadius);
		}
		FallVelocity = 0.f;
		break;

	case ECompanionState::Thrown:
		FallVelocity = 0.f;
		break;
	}
}

void ABounceCompanion::UseAbility_Implementation(AActor* User)
{
	switch (GetCompanionState())
	{
	case ECompanionState::Held:
		DropUnderHolder(User);
		break;

	case ECompanionState::Idle:
		StartPulse(User); // Remote use: the player can be anywhere
		break;

	case ECompanionState::Thrown:
	default:
		break; // Nothing while in mid-air
	}
}

void ABounceCompanion::DropUnderHolder(AActor* User)
{
	ACharacter* Character = Cast<ACharacter>(User ? User : GetHolder());
	if (!Character)
	{
		return;
	}

	// Only works while the player is standing on the ground
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (!Move || !Move->IsMovingOnGround())
	{
		return;
	}

	const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight());
	Drop(Feet + FVector(0.f, 0.f, BaseRadius + 1.f));

	// explode after dropping
	StartPulse(User);
}

void ABounceCompanion::StartPulse(AActor* User)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastPulseTime < PulseCooldown)
	{
		return;
	}

	LastPulseTime = Now;
	PulseTimeRemaining = PulseDuration;

	OnPulseStarted();

	// If the player is close enough, launch them away from the companion
	ACharacter* Character = Cast<ACharacter>(User);
	if (!Character)
	{
		return;
	}

	const FVector Offset = Character->GetActorLocation() - GetActorLocation();
	const float Dist = Offset.Size();
	if (Dist > PlayerLaunchRange)
	{
		return;
	}

	// Directly above the companion launches straight up; off to the side launches up and away
	FVector Dir = Dist > KINDA_SMALL_NUMBER ? Offset / Dist : FVector::UpVector;
	Dir.Z = FMath::Max(Dir.Z, MinLaunchUpComponent);
	Dir = Dir.GetSafeNormal();

	const float Alpha = FMath::GetMappedRangeValueClamped(
		FVector2D(0.f, PlayerLaunchRange), FVector2D(1.f, LaunchFalloffAtMaxRange), Dist);

	Character->LaunchCharacter(Dir * LaunchStrength * Alpha, bLaunchOverridesXY, /*bZOverride=*/true);
	OnLaunchedPlayer(Character);
}

void ABounceCompanion::UpdateCrawl(float DeltaSeconds)
{
	AActor* Target = GetFollowTarget();
	if (Target)
	{
		const FVector MyLoc = GetActorLocation();
		FVector ToTarget = Target->GetActorLocation() - MyLoc;
		ToTarget.Z = 0.f;
		const float Dist = ToTarget.Size();

		if (Dist > TeleportDistance)
		{
			// Too far behind. Pop back in beside the target.
			FVector NewLoc = Target->GetActorLocation() - Target->GetActorForwardVector() * FollowDistance;
			SetActorLocation(NewLoc, false, nullptr, ETeleportType::TeleportPhysics);
			FallVelocity = 0.f;
		}
		else if (Dist > FollowDistance)
		{
			const FVector Dir = ToTarget / Dist;
			const float Step = FMath::Min(CrawlSpeed * DeltaSeconds, Dist - FollowDistance);

			AddActorWorldOffset(Dir * Step, /*bSweep=*/true);

			const FRotator Desired = FRotator(0.f, Dir.Rotation().Yaw, 0.f);
			SetActorRotation(FMath::RInterpTo(GetActorRotation(), Desired, DeltaSeconds, TurnSpeed));
		}
	}

	SnapToFloor(DeltaSeconds);
}

void ABounceCompanion::SnapToFloor(float DeltaSeconds)
{
	const float Radius = Collision->GetScaledSphereRadius();
	FVector Loc = GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrawlerFloor), false, this);
	if (AActor* Target = GetFollowTarget())
	{
		Params.AddIgnoredActor(Target);
	}

	const FVector Start = Loc + FVector(0.f, 0.f, FloorTraceUp);
	const FVector End = Loc - FVector(0.f, 0.f, Radius + FloorTraceDown);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		FallVelocity = 0.f;
		const float DesiredZ = Hit.ImpactPoint.Z + Radius + 1.f;
		Loc.Z = FMath::FInterpTo(Loc.Z, DesiredZ, DeltaSeconds, FloorSnapSpeed);
	}
	else
	{
		// Ledge or void: fall until we find ground again
		FallVelocity += GravityZ * DeltaSeconds;
		Loc.Z += FallVelocity * DeltaSeconds;
	}

	SetActorLocation(Loc);
}

void ABounceCompanion::ApplyPush(float DeltaSeconds)
{
	const float PushRadius = Collision->GetScaledSphereRadius() + PushRadiusPadding;
	const FVector MyLoc = GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrawlerPush), false, this);
	if (AActor* Target = GetFollowTarget())
	{
		Params.AddIgnoredActor(Target); // never shove the player we follow
	}

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(Overlaps, MyLoc, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(PushRadius), Params);

	TSet<APawn*> Handled;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		APawn* Other = Cast<APawn>(Overlap.GetActor());
		if (!Other || Other == this || Other->IsA<ACompanionPawn>() || Handled.Contains(Other))
		{
			continue;
		}
		Handled.Add(Other);

		FVector Dir = Other->GetActorLocation() - MyLoc;
		Dir.Z = 0.f;
		const float Dist = Dir.Size();
		Dir = Dist > KINDA_SMALL_NUMBER ? Dir / Dist : GetActorForwardVector();

		// Stronger the closer they are to the center
		const float Alpha = FMath::GetMappedRangeValueClamped(FVector2D(0.f, PushRadius), FVector2D(1.f, 0.35f), Dist);

		UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Other->GetRootComponent());
		if (RootPrim && RootPrim->IsSimulatingPhysics())
		{
			RootPrim->AddForce(Dir * PushSpeed * Alpha * PhysicsPushAccelScale, NAME_None, /*bAccelChange=*/true);
		}
		else
		{
			// Direct shove. Reliable against character movement friction, and walks
			// enemies straight off platforms (they fall once the floor is gone).
			Other->AddActorWorldOffset(Dir * PushSpeed * Alpha * DeltaSeconds, /*bSweep=*/true);
		}
	}
}