// CompanionCarryComponent.h
// Add to the first person player character. Handles:
//  - owning up to 3 companions in slots (select with 1-3)
//  - grabbing, holding and throwing a companion
//  - forwarding "use ability" (left click) to the SELECTED companion, whether it is
//    in the player's hand or not (remote control)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CompanionPawn.h"
#include "CompanionCarryComponent.generated.h"

class ACompanionPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSelectedCompanionChanged, int32, SlotIndex, ACompanionPawn*, Companion);

UCLASS(ClassGroup = (Companion), meta = (BlueprintSpawnableComponent))
class THATGAMEJAM2_API UCompanionCarryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCompanionCarryComponent();

	// ---- Selection (bind keys 1-3) ----

	/** Select a companion slot. 0-based: key 1 -> 0, key 2 -> 1, key 3 -> 2. */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SelectCompanion(int32 SlotIndex);

	/** Add an already-placed companion to the first free slot. Returns false if full. */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	bool RegisterCompanion(ACompanionPawn* Companion);

	UFUNCTION(BlueprintPure, Category = "Companion")
	ACompanionPawn* GetSelectedCompanion() const;

	UFUNCTION(BlueprintPure, Category = "Companion")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	// ---- Follow / stay ----

	/** Make the selected companion follow the player (true) or stay where it is (false). */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SetSelectedCompanionFollowing(bool bFollow);

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void ToggleSelectedCompanionFollowing();

	/** Same for every companion in the slots. */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SetAllCompanionsFollowing(bool bFollow);

	// ---- Actions ----

	/** Bind to left click: triggers the selected companion's ability (held or not). */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void UseSelectedAbility();

	/** Bind to a key (e.g. E / right click): picks up what you look at, or throws what you hold. */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void PickUpOrThrow();

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void TryPickUp();

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void ThrowHeld();

	UFUNCTION(BlueprintPure, Category = "Companion")
	ACompanionPawn* GetHeldCompanion() const { return HeldCompanion; }

	/** Override where companions attach (defaults to the owner's camera, else root). */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SetHoldAttachComponent(USceneComponent* NewAttach) { HoldAttach = NewAttach; }

	UPROPERTY(BlueprintAssignable, Category = "Companion")
	FSelectedCompanionChanged OnSelectedCompanionChanged;

protected:
	virtual void BeginPlay() override;

	ACompanionPawn* FindCompanionToGrab() const;

	/** Clears HeldCompanion when the companion leaves the hand by itself (e.g. dropped by its ability). */
	UFUNCTION()
	void HandleCompanionStateChanged(ECompanionState OldState, ECompanionState NewState);

protected:
	/** Slots, in key order (1 = index 0). Can hold companions already placed in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Slots")
	TArray<TObjectPtr<ACompanionPawn>> Companions;

	/** Spawned next to the player at BeginPlay and added to the slots (up to MaxCompanions). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Slots")
	TArray<TSubclassOf<ACompanionPawn>> StartingCompanionClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Slots")
	int32 MaxCompanions = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Companion|Slots")
	int32 SelectedIndex = 0;

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