#include "BSWeapon.h"

#include "Net/UnrealNetwork.h"
#include "BSSharedTypes.h"
#include "GameFramework/Character.h"

ABSWeapon::ABSWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FireRate(600.0f)
	, EquippedSlot(INDEX_NONE)
{
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	WeaponMesh->SetCanEverAffectNavigation(false);

	bReplicates = true;
	bAlwaysRelevant = true;
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	SetReplicateMovement(false);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ABSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ABSWeapon, EquippedSlot, Parameters);

	Parameters.Condition = COND_SkipOwner;
	Parameters.RepNotifyCondition = REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS_FAST(ABSWeapon, bFiring, Parameters);
}

void ABSWeapon::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(false);
}

void ABSWeapon::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	GetWorldTimerManager().ClearTimer(TimerHandle_Fire);

	Super::EndPlay(EndPlayReason);
}

void ABSWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABSWeapon::OnRep_Instigator()
{
	Super::OnRep_Instigator();

	Timespan = 0.0f;

	if (GetInstigator())
	{
		WeaponMesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		WeaponMesh->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
		WeaponMesh->SetSimulatePhysics(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		SetActorTickEnabled(false);
	}
	else
	{
		SetActorHiddenInGame(false);
	}
}

float ABSWeapon::PlayEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/, float InPlayRate /*= 1.0f*/)
{
	ACharacter* const InstigatorCharacter = Cast<ACharacter>(OwningPawn);
	if (!InstigatorCharacter) return 0.f;

	if (bFirstPerson)
	{
		if (IsLocallyControlled(nullptr))
		{
			PlayMontage(WeaponAnimData.WeaponDrawMontage, InPlayRate);
			return InstigatorCharacter->PlayAnimMontage(WeaponAnimData.CharDrawMontage, InPlayRate, NAME_None);
		}
	}

	PlayMontage(WeaponAnimData.WeaponDrawMontage, InPlayRate);
	return InstigatorCharacter->PlayAnimMontage(WeaponAnimData.CharDrawMontage, InPlayRate, NAME_None);
}

void ABSWeapon::StopEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/)
{
	ACharacter* const InstigatorCharacter = Cast<ACharacter>(OwningPawn);
	if (!InstigatorCharacter) return;

	if (bFirstPerson)
	{
		if (IsLocallyControlled(nullptr))
		{
			StopMontage(WeaponAnimData.WeaponDrawMontage);
			return InstigatorCharacter->StopAnimMontage(WeaponAnimData.CharDrawMontage);
		}
	}

	StopMontage(WeaponAnimData.WeaponDrawMontage);
	return InstigatorCharacter->StopAnimMontage(WeaponAnimData.CharDrawMontage);
}

bool ABSWeapon::IsPlayingEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/)
{
	ACharacter* const InstigatorCharacter = Cast<ACharacter>(OwningPawn);
	if (!InstigatorCharacter) return false;
	UAnimInstance* const AnimInstance = InstigatorCharacter->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return false;

	if (bFirstPerson)
	{
		if (IsLocallyControlled(nullptr))
		{
			return AnimInstance->Montage_IsPlaying(WeaponAnimData.CharDrawMontage);
		}
	}

	return AnimInstance->Montage_IsPlaying(WeaponAnimData.CharDrawMontage);
}

float ABSWeapon::PlayUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/, float InPlayRate /*= 1.0f*/)
{
	ACharacter* const InstigatorCharacter = Cast<ACharacter>(OwningPawn);
	if (!InstigatorCharacter) return 0.f;

	if (bFirstPerson)
	{
		if (IsLocallyControlled(nullptr))
		{
			PlayMontage(WeaponAnimData.WeaponUnEquipMontage, InPlayRate);
			return InstigatorCharacter->PlayAnimMontage(WeaponAnimData.CharUnEquipMontage, InPlayRate, NAME_None);
		}
	}

	PlayMontage(WeaponAnimData.WeaponUnEquipMontage, InPlayRate);
	return InstigatorCharacter->PlayAnimMontage(WeaponAnimData.CharUnEquipMontage, InPlayRate, NAME_None);
}

void ABSWeapon::StopUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/)
{
	ACharacter* const InstigatorCharacter = Cast<ACharacter>(OwningPawn);
	if (!InstigatorCharacter) return;

	if (bFirstPerson)
	{
		if (IsLocallyControlled(nullptr))
		{
			StopMontage(WeaponAnimData.WeaponUnEquipMontage);
			return InstigatorCharacter->StopAnimMontage(WeaponAnimData.CharUnEquipMontage);
		}
	}

	StopMontage(WeaponAnimData.WeaponUnEquipMontage);
	return InstigatorCharacter->StopAnimMontage(WeaponAnimData.CharUnEquipMontage);
}

bool ABSWeapon::IsPlayingUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/)
{
	ACharacter* const InstigatorCharacter = Cast<ACharacter>(OwningPawn);
	if (!InstigatorCharacter) return false;
	UAnimInstance* const AnimInstance = InstigatorCharacter->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return false;

	if (bFirstPerson)
	{
		if (IsLocallyControlled(nullptr))
		{
			return AnimInstance->Montage_IsPlaying(WeaponAnimData.CharUnEquipMontage);
		}
	}

	return AnimInstance->Montage_IsPlaying(WeaponAnimData.CharUnEquipMontage);
}

void ABSWeapon::OnStartFire()
{
	if (!CanFire()) return;

	if (GetWorldTimerManager().IsTimerActive(TimerHandle_Fire)) return;

	if (!bFiring)
	{
		bFiring = true;

		if (IsLocallyControlled())
		{
			OnFiringSet(bFiring);
		}

		if (GetNetMode() == NM_Client)
		{
			ServerOnFiringSet(bFiring);
			return;
		}
	}
}

void ABSWeapon::OnStopFire()
{
	if (bFiring)
	{
		bFiring = false;

		if (IsLocallyControlled())
		{
			OnFiringSet(bFiring);
		}

		if (GetNetMode() == NM_Client)
		{
			ServerOnFiringSet(bFiring);
			return;
		}
	}
}

bool ABSWeapon::CanFire() const
{
	return true;
}

void ABSWeapon::SetEquippedSlot(int32 InEquippedSlot)
{
	if (GetNetMode() != NM_Client)
	{
		if (EquippedSlot != InEquippedSlot)
		{
			EquippedSlot = InEquippedSlot;

			OnRep_EquippedSlot();
		}
	}
}

void ABSWeapon::OnEquipped(int32 InSlot)
{
	SetEquippedSlot(InSlot);
}

void ABSWeapon::OnUnEquipped()
{
	SetEquippedSlot(INDEX_NONE);
}

void ABSWeapon::OnPickup(ACharacter* InCharacter)
{
	if (GetNetMode() != NM_Client)
	{
		SetOwner(InCharacter);
		SetInstigator(InCharacter);
		SetLifeSpan(0.0f);

		OnRep_Instigator();
	}
}

void ABSWeapon::OnDrop(ACharacter* InCharacter)
{
	if (GetNetMode() != NM_Client)
	{
		Timespan = 0.0f;

		if (InCharacter)
		{
			if (AController* Controller = InCharacter->GetController())
			{
				const FRotator& ControlRotation = Controller->GetControlRotation();

				const FVector& ForwardVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::X);
				const FVector& UpVector = FRotationMatrix(ControlRotation).GetScaledAxis(EAxis::Z);
				const FVector& CombinedVector = UpVector.RotateAngleAxis(10.0f, ForwardVector);

				DropWeapon(GetActorLocation(), GetActorRotation().GetNormalized().Vector(), CombinedVector);
			}
		}

		SetOwner(nullptr);
		SetInstigator(nullptr);

		OnRep_Instigator();

		OnUnEquipped();
	}
}

void ABSWeapon::OnAdded(ACharacter* InCharacter)
{

}

void ABSWeapon::OnRemoved(ACharacter* InCharacter)
{

}

void ABSWeapon::OnFired()
{
	if (!CanFire() || !bFiring)
	{
		return;
	}

	OnWeaponFiring.ExecuteIfBound();

	const float Delay = 60.0f / FireRate;
	GetWorldTimerManager().SetTimer(TimerHandle_Fire, this, &ABSWeapon::OnFired, Delay, false);
}

void ABSWeapon::OnFiring()
{
	// Diminuir munição?

	if (!IsLocallyControlled())
	{
		OnWeaponFiring.ExecuteIfBound();
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		OnFiredFX();
	}
}

void ABSWeapon::OnFiredFX()
{
	if (ACharacter* OwningCharacter = GetInstigator<ACharacter>())
	{
		OwningCharacter->PlayAnimMontage(WeaponAnimData.FireMontage);
	}
}

void ABSWeapon::OnFiringSet(bool bIsFiring)
{
	if (bIsFiring)
	{
		// Play fire sound
	}
	else
	{
		// Stop fire sound
	}

	OnWeaponFiringSet.ExecuteIfBound(bIsFiring);
}

void ABSWeapon::ServerOnFiringSet_Implementation(bool bIsFiring)
{
	if (bFiring != bIsFiring)
	{
		bFiring = bIsFiring;
	}
}

void ABSWeapon::PlayMontage(UAnimMontage* Montage, float PlayRate /*= 1.f*/) const
{
	if (UAnimInstance* AnimInstance = WeaponMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Play(Montage, PlayRate);
	}
}

void ABSWeapon::StopMontage(UAnimMontage* Montage /*= nullptr*/, float BlendOut /*= 0.1f*/) const
{
	if (UAnimInstance* AnimInstance = WeaponMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(BlendOut, Montage);
	}
}

FTransform ABSWeapon::GetMuzzleTransform() const
{
	if (!::IsValid(WeaponMesh)) return FTransform::Identity;

	return WeaponMesh->GetSocketTransform(TEXT("Muzzle"));
}

FVector ABSWeapon::GetPivotOffset() const
{
	if (!::IsValid(WeaponMesh)) return FVector::ZeroVector;

	FVector WeaponPivotOffset = WeaponMesh->GetSocketTransform(TEXT("PivotPoint"), RTS_Component).GetLocation();
	WeaponPivotOffset.Z *= -1.0f;
	return WeaponPivotOffset;
}

void ABSWeapon::DropWeapon_Implementation(FVector_NetQuantize InLocation, FVector_NetQuantizeNormal InRotation, FVector_NetQuantizeNormal InDirection)
{
	Timespan = 0.0f;

	SetActorHiddenInGame(false);

	PreviousLocation = WeaponMesh->GetComponentLocation();
	SetActorTickEnabled(GetNetMode() != NM_DedicatedServer);
	SetActorLocationAndRotation(InLocation, InRotation.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	WeaponMesh->SetCollisionProfileName(TEXT("GroundedItem"));
	WeaponMesh->SetMassOverrideInKg(NAME_None, 10.0f);

	if (GetAttachParentActor())
	{
		DetachFromActor(FDetachmentTransformRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepRelative, false));
	}

	if (GetNetMode() == NM_DedicatedServer) return;

	WeaponMesh->SetSimulatePhysics(true);

	const FVector Impulse = InDirection * 50.0f * WeaponMesh->GetMass();
	const FVector AngularVelocity = FRotationMatrix(InRotation.Rotation()).GetScaledAxis(EAxis::X) * 10.0f;

	WeaponMesh->SetAllPhysicsLinearVelocity(Impulse);
	WeaponMesh->SetAllPhysicsAngularVelocityInRadians(AngularVelocity);
}

bool ABSWeapon::IsLocallyControlled(APawn* InPawn /*= nullptr*/) const
{
	APawn* TargetPawn = InPawn ? InPawn : GetInstigator();
	return TargetPawn ? TargetPawn->IsLocallyControlled() : false;
}

bool ABSWeapon::IsPlayerControlled(APawn* InPawn /*= nullptr*/) const
{
	APawn* TargetPawn = InPawn ? InPawn : GetInstigator();
	return TargetPawn ? TargetPawn->IsPlayerControlled() : false;
}

void ABSWeapon::OnRep_EquippedSlot()
{
	if (!GetInstigator()) return;

	UE_LOG(LogTemp, Verbose, TEXT("%s"), *BS_LOGS_LINE);

	const bool bIsEquipped = EquippedSlot != INDEX_NONE;

	// ...
}

void ABSWeapon::OnRep_Firing()
{
	if (!IsLocallyControlled())
	{
		OnFiringSet(bFiring);
	}
}
