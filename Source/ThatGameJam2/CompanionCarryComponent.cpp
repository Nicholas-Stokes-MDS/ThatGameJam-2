// CompanionCarryComponent.cpp

#include "CompanionCarryComponent.h"
#include "CompanionPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

UCompanionCarryComponent::UCompanionCarryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCompanionCarryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!HoldAttach && GetOwner())
	{
		if (UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>())
		{
			HoldAttach = Camera;
		}
		else
		{
			HoldAttach = GetOwner()->GetRootComponent();
		}
	}
}

void UCompanionCarryComponent::PickUpOrThrow()
{
	if (HeldCompanion)
	{
		ThrowHeld();
	}
	else
	{
		TryPickUp();
	}
}

void UCompanionCarryComponent::TryPickUp()
{
	if (HeldCompanion || !HoldAttach)
	{
		return;
	}

	if (ACompanionPawn* Companion = FindCompanionToGrab())
	{
		Companion->PickUp(GetOwner(), HoldAttach, HoldOffset);
		if (Companion->IsHeld())
		{
			HeldCompanion = Companion;
		}
	}
}

void UCompanionCarryComponent::ThrowHeld()
{
	if (!HeldCompanion)
	{
		return;
	}

	FVector EyeLoc;
	FRotator EyeRot;
	GetOwner()->GetActorEyesViewPoint(EyeLoc, EyeRot);

	const FVector Velocity = EyeRot.Vector() * ThrowSpeed
		+ GetOwner()->GetVelocity()
		+ FVector(0.f, 0.f, ThrowUpwardBoost);

	HeldCompanion->Throw(Velocity);
	HeldCompanion = nullptr;
}

void UCompanionCarryComponent::UseHeldAbility()
{
	if (HeldCompanion)
	{
		HeldCompanion->UseAbility(GetOwner());
	}
}

ACompanionPawn* UCompanionCarryComponent::FindCompanionToGrab() const
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return nullptr;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(CompanionPickup), false, Owner);

	// 1) What the player is looking at
	FVector EyeLoc;
	FRotator EyeRot;
	Owner->GetActorEyesViewPoint(EyeLoc, EyeRot);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByChannel(Hits, EyeLoc, EyeLoc + EyeRot.Vector() * PickupRange, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(PickupRadius), Params);

	for (const FHitResult& Hit : Hits)
	{
		ACompanionPawn* Companion = Cast<ACompanionPawn>(Hit.GetActor());
		if (Companion && Companion->CanBePickedUp())
		{
			return Companion;
		}
	}

	// 2) Fallback: closest companion near the player's feet
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(Overlaps, Owner->GetActorLocation(), FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(NearbyPickupRadius), Params);

	ACompanionPawn* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		ACompanionPawn* Companion = Cast<ACompanionPawn>(Overlap.GetActor());
		if (Companion && Companion->CanBePickedUp())
		{
			const float DistSq = FVector::DistSquared(Companion->GetActorLocation(), Owner->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Companion;
			}
		}
	}
	return Best;
}
