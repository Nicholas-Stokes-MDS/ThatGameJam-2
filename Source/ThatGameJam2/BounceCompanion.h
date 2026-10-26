// CrawlerCompanion.h
// Grounded companion. Crawls near the player's feet.
//  - In hand:     UseAbility launches the player upward.
//  - Not in hand: expands and shoves nearby pawns away (enemies off ledges, etc.)

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

	/** Hook for VFX/SFX/animation when the player gets launched. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crawler")
	void OnLaunchedHolder(AActor* LaunchedActor);

protected:
	virtual void BeginPlay() override;
	virtual void ApplyIdleCollision() override;

	void UpdateCrawl(float DeltaSeconds);
	void SnapToFloor(float DeltaSeconds);
	void ApplyPush(float DeltaSeconds);

protected:
	// ---- Crawling ----

	/** Crawl speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float CrawlSpeed = 350.f;

	/** Stays within this planar distance of the follow target's position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float FollowDistance = 120.f;

	/** If further than this, teleport next to the follow target (fell off the map, got left behind). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float TeleportDistance = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float TurnSpeed = 10.f;

	/** How far above its center the floor trace starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float FloorTraceUp = 30.f;

	/** How far below its bottom the floor trace reaches. Beyond this it falls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float FloorTraceDown = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float FloorSnapSpeed = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Movement")
	float GravityZ = -980.f;

	// ---- Launch ability (while held) ----

	/** Upward launch velocity applied to the player (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Launch")
	float LaunchStrength = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Launch")
	float LaunchCooldown = 0.6f;

	/** If true the launch replaces the player's horizontal velocity, otherwise it keeps it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Launch")
	bool bLaunchOverridesXY = false;

	// ---- Expand and push (while not held) ----

	/** Visual (and collision) scale multiplier when out of the player's hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Push")
	float ExpandedScale = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Push")
	float ExpandInterpSpeed = 6.f;

	/** Extra reach beyond the expanded body for pushing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Push")
	float PushRadiusPadding = 40.f;

	/** Speed (cm/s) at which overlapping pawns are shoved away, at the point of strongest push. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Push")
	float PushSpeed = 450.f;

	/** Multiplier turning PushSpeed into an acceleration for physics-simulating pawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crawler|Push")
	float PhysicsPushAccelScale = 10.f;

private:
	float BaseRadius = 25.f;
	float CurrentScale = 1.f;
	float FallVelocity = 0.f;
	double LastLaunchTime = -1000.0;
};