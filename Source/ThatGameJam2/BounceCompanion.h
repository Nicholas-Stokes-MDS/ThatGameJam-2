// BounceCompanion.h
// Grounded companion. Crawls near the player's feet.
//  - UseAbility while held (player grounded): drops the companion at the player's feet.
//  - UseAbility while not held: expands for a moment and launches every pawn in range
//                               (player, enemies, other companions) away from it.
//                               Works like a remote control, the player can be anywhere.
// It never expands on its own, only when the ability is triggered.

#pragma once

#include "CoreMinimal.h"
#include "CompanionPawn.h"
#include "BounceCompanion.generated.h"

UCLASS()
class THATGAMEJAM2_API ABounceCompanion : public ACompanionPawn
{
	GENERATED_BODY()

public:
	ABounceCompanion();

	virtual void Tick(float DeltaSeconds) override;

	virtual void UseAbility_Implementation(AActor* User) override;

	UFUNCTION(BlueprintPure, Category = "Bounce")
	bool IsPulsing() const { return PulseTimeRemaining > 0.f; }

	/** Hook for VFX/SFX/animation when the pulse launches the player. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Bounce")
	void OnLaunchedPlayer(AActor* LaunchedActor);

	/** Hook for VFX/SFX/animation when the expand pulse starts (remote ability). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Bounce")
	void OnPulseStarted();

protected:
	virtual void BeginPlay() override;
	virtual void ApplyIdleCollision() override;
	virtual void OnStateChanged(ECompanionState OldState, ECompanionState NewState) override;

	void DropUnderHolder(AActor* User);
	void StartPulse(AActor* User);

	/** Launches every pawn in range (player, enemies, other companions) away from this companion. */
	void LaunchPawnsInRange(AActor* User);

	/** Launch velocity for something at TargetLoc. False if it is out of range. */
	bool ComputeLaunchVelocity(const FVector& TargetLoc, FVector& OutVelocity) const;

	void UpdateCrawl(float DeltaSeconds);
	void SnapToFloor(float DeltaSeconds);
	void ApplyPush(float DeltaSeconds);

protected:
	// ---- Crawling ----

	/** Crawl speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Movement")
	float CrawlSpeed = 350.f;

	/** Stays within this planar distance of the follow target's position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Movement")
	float FollowDistance = 120.f;

	/** If further than this, teleport next to the follow target (fell off the map, got left behind). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Movement")
	float TeleportDistance = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Movement")
	float TurnSpeed = 10.f;

	/** How far above its center the floor trace starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Movement")
	float FloorTraceUp = 30.f;

	/** How far below its bottom the floor trace reaches. Beyond this it falls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Movement")
	float FloorTraceDown = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Movement")
	float FloorSnapSpeed = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Movement")
	float GravityZ = -980.f;

	// ---- Launch (pulse) ----

	/** Launch speed (cm/s) at point blank, applied away from the companion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Launch")
	float LaunchStrength = 1200.f;

	/** Any pawn (player, enemy, other companion) is launched only if its center is within this distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Launch")
	float PlayerLaunchRange = 250.f;

	/** Launch strength multiplier at the edge of PlayerLaunchRange (1 = no falloff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Launch")
	float LaunchFalloffAtMaxRange = 0.5f;

	/** The launch direction always has at least this much upward component (0 to 1), so side hits still pop things up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Launch")
	float MinLaunchUpComponent = 0.3f;

	/** Characters only: if true the launch replaces their current velocity, otherwise it is added to it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Launch")
	bool bLaunchOverridesXY = true;

	// ---- Expand pulse (ability while not held) ----

	/** Scale multiplier at full expansion (visual and collision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	float ExpandedScale = 3.f;

	/** How fast it grows when triggered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	float ExpandInterpSpeed = 12.f;

	/** How fast it shrinks back once the pulse is over. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	float ContractInterpSpeed = 5.f;

	/** How long (seconds) it stays expanded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	float PulseDuration = 0.6f;

	/** Minimum time between pulses, measured from the moment of triggering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	float PulseCooldown = 1.5f;

	/** Also keep shoving overlapping pawns for the whole pulse (in addition to the launch at the start). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	bool bSustainedPush = false;

	/** Extra reach beyond the expanded body for the sustained push. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	float PushRadiusPadding = 40.f;

	/** Speed (cm/s) at which overlapping pawns are shoved away, at the point of strongest push. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	float PushSpeed = 800.f;

	/** Multiplier turning PushSpeed into an acceleration for physics-simulating pawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bounce|Pulse")
	float PhysicsPushAccelScale = 10.f;

private:
	float BaseRadius = 25.f;
	float CurrentScale = 1.f;
	float FallVelocity = 0.f;
	float PulseTimeRemaining = 0.f;
	double LastPulseTime = -1000.0;
};