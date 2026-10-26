// CompanionCarryComponent.h
// Add to the first person player character. Handles grabbing, holding, throwing
// and forwarding the "use ability" input to the held companion.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CompanionCarryComponent.generated.h"

class ACompanionPawn;

UCLASS(ClassGroup = (Companion), meta = (BlueprintSpawnableComponent))
class THATGAMEJAM2_API UCompanionCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCompanionCarryComponent();

	/** Bind to a key (e.g. E / right click): picks up what you look at, or throws what you hold. */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void PickUpOrThrow();

	/** Bind to left click: triggers the held companion's ability. */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void UseHeldAbility();

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void TryPickUp();

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void ThrowHeld();

	UFUNCTION(BlueprintPure, Category = "Companion")
	ACompanionPawn* GetHeldCompanion() const { return HeldCompanion; }

	/** Override where companions attach (defaults to the owner's camera, else root). */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SetHoldAttachComponent(USceneComponent* NewAttach) { HoldAttach = NewAttach; }

protected:
	virtual void BeginPlay() override;

	ACompanionPawn* FindCompanionToGrab() const;

protected:
	/** How far in front of the camera you can grab. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Pickup")
	float PickupRange = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Pickup")
	float PickupRadius = 40.f;

	/** Fallback: also grab any companion within this distance of the player (e.g. one at your feet). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Pickup")
	float NearbyPickupRadius = 180.f;

	/** Hand position relative to the camera (X forward, Y right, Z up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Hold")
	FVector HoldOffset = FVector(70.f, 30.f, -30.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Throw")
	float ThrowSpeed = 1400.f;

	/** Extra upward velocity added to throws. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Throw")
	float ThrowUpwardBoost = 150.f;

	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	TObjectPtr<ACompanionPawn> HeldCompanion;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> HoldAttach;
};
