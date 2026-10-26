// CompanionPawn.h
// Base class for companion creatures that the player can pick up, throw and trigger an ability on. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CompanionPawn.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ECompanionState : uint8
{
	Idle,	// On the ground, doing its own thing
	Held,	// In the player's hand
	Thrown	// Flying through the air (physics driven)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCompanionStateChanged, ECompanionState, OldState, ECompanionState, NewState);

UCLASS(Abstract)
class THATGAMEJAM2_API ACompanionPawn : public APawn
{
	GENERATED_BODY()

public:
	ACompanionPawn();

	virtual void Tick(float DeltaSeconds) override;

	// ---- Player-facing API (called by UCompanionCarryComponent) ----

	/** Can the player grab this right now? */
	UFUNCTION(BlueprintPure, Category = "Companion")
	virtual bool CanBePickedUp() const { return State == ECompanionState::Idle; }

	/** Attach to the holder (usually the first person camera). */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void PickUp(AActor* NewHolder, USceneComponent* AttachParent, FVector RelativeOffset);

	/** Detach from the holder and fly with the given velocity (cm/s). */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void Throw(FVector Velocity);

	/** Detach from the holder and place at a world location, standing on its own (Idle). */
	UFUNCTION(BlueprintCallable, Category = "Companion")
	void Drop(FVector WorldLocation);

	/** Triggered by right click when not held. Override in children (C++ or Blueprint). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Companion")
	void UseAbility(AActor* User);
	virtual void UseAbility_Implementation(AActor* User);

	// ---- Accessors ----

	UFUNCTION(BlueprintPure, Category = "Companion")
	ECompanionState GetCompanionState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Companion")
	bool IsHeld() const { return State == ECompanionState::Held; }

	UFUNCTION(BlueprintPure, Category = "Companion")
	AActor* GetHolder() const { return Holder; }

	/** The actor this companion follows when idle. Set automatically on first pickup; can also be set on spawn. */
	UFUNCTION(BlueprintPure, Category = "Companion")
	AActor* GetFollowTarget() const { return FollowTarget; }

	UFUNCTION(BlueprintCallable, Category = "Companion")
	void SetFollowTarget(AActor* NewTarget) { FollowTarget = NewTarget; }

	UPROPERTY(BlueprintAssignable, Category = "Companion")
	FCompanionStateChanged OnCompanionStateChanged;

protected:
	virtual void BeginPlay() override;

	/** Called after the state changed. Override for child-specific reactions. */
	virtual void OnStateChanged(ECompanionState OldState, ECompanionState NewState) {}

	/** Collision setup while idle. Children can extend (call Super first). */
	virtual void ApplyIdleCollision();

	void SetState(ECompanionState NewState);
	void ApplyCollisionForState(ECompanionState NewState);

	/** End of flight: turn physics off, stand upright, become Idle. */
	void Settle();

	UFUNCTION()
	void OnCollisionHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void RestorePawnCollision();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	ECompanionState State = ECompanionState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	TObjectPtr<AActor> Holder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TObjectPtr<AActor> FollowTarget;

	/** Seconds after a throw during which the companion ignores pawns (so it doesn't hit the thrower). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Throw")
	float ThrowIgnorePawnTime = 0.25f;

	/** Minimum surface normal Z to count as ground when landing (0.6 is about a 53 degree slope). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Throw")
	float LandingNormalZ = 0.6f;

	/** Safety net: settle after this long even if no ground hit was registered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Throw")
	float MaxThrownTime = 5.f;

	/** Settle once slower than this (cm/s) after MinThrownTime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Throw")
	float SettleSpeed = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Throw")
	float MinThrownTime = 0.3f;

private:
	float ThrownTime = 0.f;
	FTimerHandle PawnCollisionTimer;
};