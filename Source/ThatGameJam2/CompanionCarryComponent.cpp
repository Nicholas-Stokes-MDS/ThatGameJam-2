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

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (!HoldAttach)
	{
		if (UCameraComponent* Camera = Owner->FindComponentByClass<UCameraComponent>())
		{
			HoldAttach = Camera;
		}
		else
		{
			HoldAttach = Owner->GetRootComponent();
		}
	}

	// Slots filled with level-placed companions follow the player
	for (ACompanionPawn* Companion : Companions)
	{
		if (Companion && !Companion->GetFollowTarget())
		{
			Companion->SetFollowTarget(Owner);
		}
	}

	// Spawn any starting companions next to the player
	if (UWorld* World = GetWorld())
	{
		for (int32 i = 0; i < StartingCompanionClasses.Num(); ++i)
		{
			if (Companions.Num() >= MaxCompanions)
			{
				break;
			}
			if (!StartingCompanionClasses[i])
			{
				continue;
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			Params.Owner = Owner;

			const FVector Loc = Owner->GetActorLocation()
				+ Owner->GetActorForwardVector() * 150.f
				+ Owner->GetActorRightVector() * (100.f * (Companions.Num() - 1));

			if (ACompanionPawn* Spawned = World->SpawnActor<ACompanionPawn>(StartingCompanionClasses[i], Loc, Owner->GetActorRotation(), Params))
			{
				RegisterCompanion(Spawned);
			}
		}
	}

	SelectedIndex = FMath::Clamp(SelectedIndex, 0, FMath::Max(0, Companions.Num() - 1));
}

bool UCompanionCarryComponent::RegisterCompanion(ACompanionPawn* Companion)
{
	if (!Companion || Companions.Contains(Companion) || Companions.Num() >= MaxCompanions)
	{
		return false;
	}

	Companions.Add(Companion);
	if (GetOwner())
	{
		Companion->SetFollowTarget(GetOwner());
	}
	return true;
}

void UCompanionCarryComponent::SelectCompanion(int32 SlotIndex)
{
	if (!Companions.IsValidIndex(SlotIndex) || !Companions[SlotIndex])
	{
		return;
	}

	SelectedIndex = SlotIndex;
	OnSelectedCompanionChanged.Broadcast(SelectedIndex, Companions[SlotIndex]);
}

ACompanionPawn* UCompanionCarryComponent::GetSelectedCompanion() const
{
	return Companions.IsValidIndex(SelectedIndex) ? Companions[SelectedIndex].Get() : nullptr;
}

void UCompanionCarryComponent::UseSelectedAbility()
{
	if (ACompanionPawn* Selected = GetSelectedCompanion())
	{
		Selected->UseAbility(GetOwner());
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