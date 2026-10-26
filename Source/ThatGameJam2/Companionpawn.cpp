// CompanionPawn.cpp

#include "CompanionPawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

ACompanionPawn::ACompanionPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled; // We drive movement ourselves, no controller needed

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(25.f);
	Collision->SetCollisionProfileName(TEXT("Pawn"));
	SetRootComponent(Collision);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACompanionPawn::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentHit.AddDynamic(this, &ACompanionPawn::OnCollisionHit);
	ApplyCollisionForState(State);
}

void ACompanionPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == ECompanionState::Thrown)
	{
		ThrownTime += DeltaSeconds;

		const bool bTimedOut = ThrownTime >= MaxThrownTime;
		const bool bSlow = ThrownTime >= MinThrownTime &&
			Collision->GetPhysicsLinearVelocity().SizeSquared() < FMath::Square(SettleSpeed);

		if (bTimedOut || bSlow)
		{
			Settle();
		}
	}
}

void ACompanionPawn::PickUp(AActor* NewHolder, USceneComponent* AttachParent, FVector RelativeOffset)
{
	if (!CanBePickedUp() || !AttachParent)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PawnCollisionTimer);

	Holder = NewHolder;
	SetOwner(NewHolder);
	if (!FollowTarget)
	{
		FollowTarget = NewHolder;
	}

	Collision->SetSimulatePhysics(false);
	SetState(ECompanionState::Held);

	AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorRelativeLocation(RelativeOffset);
	SetActorRelativeRotation(FRotator::ZeroRotator);
}

void ACompanionPawn::Throw(FVector Velocity)
{
	if (State != ECompanionState::Held)
	{
		return;
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	Holder = nullptr;
	ThrownTime = 0.f;
	SetState(ECompanionState::Thrown); // Sets up physics collision

	Collision->SetSimulatePhysics(true);
	Collision->SetPhysicsLinearVelocity(Velocity);

	GetWorldTimerManager().SetTimer(PawnCollisionTimer, this, &ACompanionPawn::RestorePawnCollision, ThrowIgnorePawnTime, false);
}

void ACompanionPawn::UseAbility_Implementation(AActor* User)
{
	// Intentionally empty. Children define what the ability does.
}

void ACompanionPawn::SetState(ECompanionState NewState)
{
	if (State == NewState)
	{
		return;
	}

	const ECompanionState OldState = State;
	State = NewState;

	ApplyCollisionForState(NewState);
	OnStateChanged(OldState, NewState);
	OnCompanionStateChanged.Broadcast(OldState, NewState);
}

void ACompanionPawn::ApplyCollisionForState(ECompanionState NewState)
{
	switch (NewState)
	{
	case ECompanionState::Held:
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;

	case ECompanionState::Thrown:
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Collision->SetCollisionProfileName(TEXT("PhysicsActor"));
		Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // restored by timer
		Collision->SetNotifyRigidBodyCollision(true);                    // needed for OnComponentHit
		break;

	case ECompanionState::Idle:
	default:
		ApplyIdleCollision();
		break;
	}
}

void ACompanionPawn::ApplyIdleCollision()
{
	// Query only: the pickup trace can find us, but we don't physically block anything.
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ACompanionPawn::Settle()
{
	if (State != ECompanionState::Thrown)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PawnCollisionTimer);

	Collision->SetSimulatePhysics(false);

	FRotator Upright = GetActorRotation();
	Upright.Pitch = 0.f;
	Upright.Roll = 0.f;
	SetActorRotation(Upright);

	SetState(ECompanionState::Idle);
}

void ACompanionPawn::OnCollisionHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (State == ECompanionState::Thrown && ThrownTime > 0.05f && Hit.ImpactNormal.Z >= LandingNormalZ)
	{
		Settle();
	}
}

void ACompanionPawn::RestorePawnCollision()
{
	if (State == ECompanionState::Thrown)
	{
		Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}
}