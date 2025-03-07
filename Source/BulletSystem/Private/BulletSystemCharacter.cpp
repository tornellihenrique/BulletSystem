// Copyright Epic Games, Inc. All Rights Reserved.

#include "BulletSystemCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "BSWeapon.h"
#include "BSWorldSettings.h"
#include "Net/UnrealNetwork.h"
#include "THotbarItemInterface.h"
#include "THotbarComponent.h"
#include "BSProjectile.h"
#include "Character/BSAnimInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "BSWeaponSwayComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ABulletSystemCharacter

ABulletSystemCharacter::ABulletSystemCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	bUseControllerRotationYaw = true;
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetMesh(), TEXT("head"));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	WeaponHolder = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponHolder"));
	WeaponHolder->SetupAttachment(GetMesh(), TEXT("VB WeaponPivot"));

	Hotbar = CreateDefaultSubobject<UTHotbarComponent>(TEXT("Hotbar"));
	Hotbar->bCurveUnEquipEnabled = false;

	WeaponSway = CreateDefaultSubobject<UBSWeaponSwayComponent>(TEXT("WeaponSway"));

	bReplicates = true;
	bAlwaysRelevant = true;
}

void ABulletSystemCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ABulletSystemCharacter, MainWeapon, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ABulletSystemCharacter, WeaponCount, Parameters);

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ABulletSystemCharacter, bFiring, Parameters);

	Parameters.Condition = COND_SkipOwner;
	Parameters.RepNotifyCondition = REPNOTIFY_Always;
	DOREPLIFETIME_WITH_PARAMS_FAST(ABulletSystemCharacter, BulletSpawnData, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ABulletSystemCharacter, BulletSpawnDataGauge, Parameters);
}

void ABulletSystemCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ABulletSystemCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ABulletSystemCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AnimInstance = Cast<UBSAnimInstance>(GetMesh()->GetAnimInstance());

	Hotbar->OnItemsUpdated.BindUObject(this, &ABulletSystemCharacter::OnHotbarItemsUpdated);

	GetWorldTimerManager().SetTimerForNextTick(this, &ABulletSystemCharacter::SpawnDefaultWeapons);
}

void ABulletSystemCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		if (AnimInstance && WeaponSway)
		{
			AnimInstance->MovingSway = WeaponSway->GetMovingSway();
			AnimInstance->FinalSwayRot = WeaponSway->GetFinalSwayRot();
			AnimInstance->FinalSwayLoc = WeaponSway->GetFinalSwayLoc();
		}
	}
}

void ABulletSystemCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABulletSystemCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABulletSystemCharacter::Look);

		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &ABulletSystemCharacter::Fire);

		EnhancedInputComponent->BindAction(ChangeWeaponAction, ETriggerEvent::Triggered, this, &ABulletSystemCharacter::ChangeWeapon);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ABulletSystemCharacter::OnRep_PlayerState()
{
}

void ABulletSystemCharacter::AddControllerPitchInput(float Val)
{
	LastControllerPitchInput = Val;
	Super::AddControllerPitchInput(Val);
}

void ABulletSystemCharacter::AddControllerYawInput(float Val)
{
	LastControllerYawInput = Val;
	Super::AddControllerYawInput(Val);
}

void ABulletSystemCharacter::OnItemStartedChange(AActor* Item, AActor* OldItem)
{
	OnActionStarted();
}

void ABulletSystemCharacter::OnItemChange(AActor* Item, AActor* OldItem)
{
	if (Item)
	{
		EquipWeapon();
	}
	else
	{
		UnEquipWeapon();
	}
}

void ABulletSystemCharacter::OnItemEndedChange(AActor* Item, AActor* OldItem)
{
	// Nope
}

void ABulletSystemCharacter::OnItemAdded(AActor* Item)
{
	if (ABSWeapon* AddedWeapon = Cast<ABSWeapon>(Item))
	{
		AddedWeapon->OnAdded(this);
	}
}

void ABulletSystemCharacter::OnItemRemoved(AActor* Item)
{
	if (ABSWeapon* AddedWeapon = Cast<ABSWeapon>(Item))
	{
		AddedWeapon->OnRemoved(this);
	}
}

bool ABulletSystemCharacter::ShouldPlayEquipAnimation() const
{
	return true;
}

bool ABulletSystemCharacter::ShouldPlayUnEquipAnimation() const
{
	return false;
}

void ABulletSystemCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void ABulletSystemCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);

		if (WeaponSway)
		{
			WeaponSway->OnLook(LookAxisVector);
		}
	}
}

void ABulletSystemCharacter::Fire(const FInputActionValue& Value)
{
	const bool bPressed = Value.Get<bool>();

	TryFire(bPressed);
}

void ABulletSystemCharacter::ChangeWeapon(const FInputActionValue& Value)
{
	const bool bIsNext = Value.Get<float>() > 0.0f;

	int32 TargetSelectedIndex = Hotbar->SelectedIndex;

	int32 TotalSize = Hotbar->Items.Num();

	if (TargetSelectedIndex >= TotalSize)
	{
		for (int32 i = 0; i < Hotbar->Items.Num(); ++i)
		{
			TargetSelectedIndex = bIsNext ? (TargetSelectedIndex + 1) % Hotbar->Items.Num() : ((TargetSelectedIndex - 1) + Hotbar->Items.Num()) % Hotbar->Items.Num();
			if (TargetSelectedIndex < TotalSize) continue;

			if (Hotbar->GetItem(TargetSelectedIndex))
			{
				break;
			}
		}
	}
	else
	{
		for (int32 i = 0; i < TotalSize; ++i)
		{
			TargetSelectedIndex = bIsNext ? (TargetSelectedIndex + 1) % TotalSize : ((TargetSelectedIndex - 1) + TotalSize) % TotalSize;

			if (Hotbar->GetItem(TargetSelectedIndex))
			{
				break;
			}
		}
	}

	Hotbar->SetSelectedIndex(TargetSelectedIndex);
}

void ABulletSystemCharacter::SpawnDefaultWeapons()
{
	if (GetNetMode() == NM_Client) return;

	TArray<FSoftClassPath> Weapons = DefaultWeapons.FilterByPredicate([this](const FSoftClassPath& InWeaponData)
	{
		return InWeaponData.IsValid();
	});

	const int TargetValidWeapons = Weapons.Num();

	for (int i = 0; i < TargetValidWeapons; i++)
	{
		if (Weapons.IsValidIndex(i))
		{
			const TSubclassOf<ABSWeapon>& WeaponClass = Weapons[i].TryLoadClass<ABSWeapon>();
			if (WeaponClass.Get())
			{
				static const FTransform SpawnTransform = FTransform::Identity;

				ABSWeapon* SpawnedWeapon = GetWorld()->SpawnActorDeferred<ABSWeapon>(WeaponClass, SpawnTransform, this, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (SpawnedWeapon)
				{
					// ...

					SpawnedWeapon->FinishSpawning(SpawnTransform);

					Hotbar->TryAddItem(SpawnedWeapon, i);
				}
			}
		}
	}

	ClientOnSpawnDefaultWeapons();
}

void ABulletSystemCharacter::ClientOnSpawnDefaultWeapons_Implementation()
{
}

void ABulletSystemCharacter::OnHotbarItemsUpdated()
{
}

void ABulletSystemCharacter::OnWeaponFiringSet(bool bIsFiring)
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;

	if (bIsFiring)
	{
		if (IsLocallyControlled())
		{
			Weapon->OnFired();
		}

		bFiring = true;
		if (IsLocallyControlled())
		{
			ServerSetFiring(bFiring);
		}
	}
	else
	{
		OnFireCancelled();
	}
}

void ABulletSystemCharacter::OnWeaponFiring()
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;

	if (IsLocallyControlled())
	{
		FireProjectile();
	}
}

void ABulletSystemCharacter::OnFireCancelled()
{
	if (!bFiring) return;

	bFiring = false;
	if (IsLocallyControlled())
	{
		ServerSetFiring(bFiring);
	}
}

void ABulletSystemCharacter::ServerSetFiring_Implementation(bool bIsFiring)
{
	bFiring = bIsFiring;
}

FTransform ABulletSystemCharacter::GetBulletSpawnTransform() const
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return FTransform::Identity;

	const FTransform ViewPoint = GetEyeViewPoint();
	const FTransform& MuzzleTransform = Weapon->GetMuzzleTransform();

	return FTransform(ViewPoint.GetRotation(), MuzzleTransform.GetLocation());
}

FTransform ABulletSystemCharacter::GetEyeViewPoint() const
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetActorEyesViewPoint(ViewLocation, ViewRotation);

	return FTransform(ViewRotation, ViewLocation);
}

void ABulletSystemCharacter::FireProjectile()
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;

	switch (Weapon->ProjectileType)
	{
		case EProjectileType::EPT_Cartridge: return SpawnBullet();
		case EProjectileType::EPT_Gauge: return SpawnGauge(Weapon->GaugePelletsCount);
	}
}

void ABulletSystemCharacter::SpawnBullet()
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;

	const FTransform& BulletSpawnTransform = GetBulletSpawnTransform();

	PerformSpawnBullet(BulletSpawnTransform.GetLocation(), BulletSpawnTransform.GetRotation().Vector());
	ServerPerformSpawnBullet(BulletSpawnTransform.GetLocation(), BulletSpawnTransform.GetRotation().Vector());
}

void ABulletSystemCharacter::SpawnGauge(int32 PelletsCount)
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;

	const FTransform& BulletSpawnTransform = GetBulletSpawnTransform();

	TArray<FVector_NetQuantizeNormal> ShootDirections;
	const FVector2D SpreadSize(2.0f, 2.0f);
	for (int i = 0; i < PelletsCount; ++i)
	{
		const float RandomX = FMath::FRandRange(-SpreadSize.X, SpreadSize.X);
		const float MaxY = FMath::Sqrt(1.f - (RandomX * RandomX) / (SpreadSize.X * SpreadSize.X)) * FMath::Pow(SpreadSize.Y, 2);
		const float RandomY = FMath::FRandRange(-MaxY, MaxY);

		const FRotator RotOffset(RandomY, RandomX, 0.0f);
		const FVector ShootDirection = (BulletSpawnTransform.GetRotation().Rotator() + RotOffset).GetNormalized().Vector();

		ShootDirections.Add(ShootDirection);
	}

	PerformSpawnGauge(BulletSpawnTransform.GetLocation(), ShootDirections);
	ServerPerformSpawnGauge(BulletSpawnTransform.GetLocation(), ShootDirections);
}

void ABulletSystemCharacter::PerformSpawnBullet(FVector_NetQuantize100 InLocation, FVector_NetQuantizeNormal InDirection)
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;

	if (!Weapon->ProjectileClass) return;

	const FTransform SpawnTransform(InDirection.Rotation(), InLocation);
	const FString& ProjectileBlueprintClassName = FPaths::GetBaseFilename(Weapon->ProjectileClass->GetPathName());

	if (ABSWorldSettings* WS = Cast<ABSWorldSettings>(GetWorld()->GetWorldSettings()))
	{
		if (ABSProjectile* Projectile = WS->GetArrayPoolActor<ABSProjectile>(*ProjectileBlueprintClassName))
		{
			Projectile->SetOwner(Weapon);
			Projectile->SetInstigator(this);

			Projectile->SetActorTransform(SpawnTransform);
			Projectile->OnRemovedFromPool();

			Weapon->OnFiring();
		}
		else
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			SpawnParams.Instigator = nullptr;
			SpawnParams.Owner = nullptr;
			SpawnParams.bNoFail = true;

			ABSProjectile* SpawnedProjectile = GetWorld()->SpawnActorDeferred<ABSProjectile>(Weapon->ProjectileClass, SpawnTransform, Weapon, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedProjectile)
			{
				// ...

				SpawnedProjectile->FinishSpawning(SpawnTransform);

				Weapon->OnFiring();
			}
		}
	}
}

void ABulletSystemCharacter::ServerPerformSpawnBullet_Implementation(FVector_NetQuantize100 InLocation, FVector_NetQuantizeNormal InDirection)
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;
	
	if (!IsLocallyControlled())
	{
		// Decrease ammo
	}

	BulletSpawnData = FBulletSpawnData(InLocation, InDirection);
}

void ABulletSystemCharacter::PerformSpawnGauge(FVector_NetQuantize100 InLocation, const TArray<FVector_NetQuantizeNormal>& InDirections)
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;

	if (!Weapon->ProjectileClass) return;

	const FString& ProjectileBlueprintClassName = FPaths::GetBaseFilename(Weapon->ProjectileClass->GetPathName());

	for (const FVector_NetQuantizeNormal& ShootDirection : InDirections)
	{
		const FTransform SpawnTransform(ShootDirection.Rotation(), InLocation);

		if (ABSWorldSettings* WS = Cast<ABSWorldSettings>(GetWorld()->GetWorldSettings()))
		{
			if (ABSProjectile* Projectile = WS->GetArrayPoolActor<ABSProjectile>(*ProjectileBlueprintClassName))
			{
				Projectile->SetOwner(Weapon);
				Projectile->SetInstigator(this);

				Projectile->SetActorTransform(SpawnTransform);
				Projectile->OnRemovedFromPool();
			}
			else
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
				SpawnParams.Instigator = nullptr;
				SpawnParams.Owner = nullptr;
				SpawnParams.bNoFail = true;

				ABSProjectile* SpawnedProjectile = GetWorld()->SpawnActorDeferred<ABSProjectile>(Weapon->ProjectileClass, SpawnTransform, Weapon, this, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (SpawnedProjectile)
				{
					// ...

					SpawnedProjectile->FinishSpawning(SpawnTransform);
				}
			}
		}
	}

	Weapon->OnFiring();
}

void ABulletSystemCharacter::ServerPerformSpawnGauge_Implementation(FVector_NetQuantize100 InLocation, const TArray<FVector_NetQuantizeNormal>& InDirections)
{
	ABSWeapon* Weapon = GetWeapon();
	if (!Weapon) return;

	if (!IsLocallyControlled())
	{
		// Decrease ammo
	}

	BulletSpawnDataGauge = FBulletSpawnDataGauge(InLocation, InDirections);
}

void ABulletSystemCharacter::InitializeWeapon()
{
	if (!GetWeapon()) return;

	if (GetWeapon()->GetRootComponent() && (!GetWeapon()->GetRootComponent()->GetAttachParent() || GetWeapon()->GetRootComponent()->GetAttachParent() != WeaponHolder))
	{
		GetWeapon()->AttachToComponent(WeaponHolder, FAttachmentTransformRules::SnapToTargetIncludingScale);
	}

	GetWeapon()->OnWeaponFiring.BindUObject(this, &ABulletSystemCharacter::OnWeaponFiring);
	GetWeapon()->OnWeaponFiringSet.BindUObject(this, &ABulletSystemCharacter::OnWeaponFiringSet);

	if (AnimInstance)
	{
		AnimInstance->BasePose = GetWeapon()->WeaponAnimData.Pose;
		//AnimInstance->PivotOffset = -GetWeapon()->GetPivotOffset();
		AnimInstance->WeaponOffset = GetWeapon()->WeaponAnimData.WeaponOffset;
	}

	GetWeapon()->SetActorRelativeRotation(FRotator::ZeroRotator);
	//WeaponHolder->SetRelativeLocation(GetWeapon()->GetPivotOffset());

	SetWeaponVisibility(false);
	GetWorldTimerManager().SetTimer(TimerHandle_WeaponVisibility, this, &ABulletSystemCharacter::RestoreWeaponVisibility, 0.04f);
}

void ABulletSystemCharacter::EquipWeapon()
{
	if (!Hotbar->HasValidSelectedIndex()) return;

	if (MainWeapon == Hotbar->GetSelectedItem())
	{
		OnRep_MainWeapon(nullptr);
		return;
	}

	if (GetNetMode() == NM_Client)
	{
		ServerEquipWeapon();
		return;
	}

	ABSWeapon* OldWeapon = MainWeapon;
	if (OldWeapon)
	{
		OldWeapon->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		OldWeapon->SetActorHiddenInGame(true);

		OldWeapon->OnUnEquipped();

		OldWeapon->OnWeaponFiring.Unbind();
		OldWeapon->OnWeaponFiringSet.Unbind();
	}

	MainWeapon = Cast<ABSWeapon>(Hotbar->GetSelectedItem());

	if (MainWeapon)
	{
		MainWeapon->OnEquipped(Hotbar->SelectedIndex);

		if (GetNetMode() != NM_DedicatedServer)
		{
			OnRep_MainWeapon(OldWeapon);
			return;
		}

		InitializeWeapon();
	}
}

void ABulletSystemCharacter::ServerEquipWeapon_Implementation()
{
	EquipWeapon();
}

void ABulletSystemCharacter::UnEquipWeapon()
{
	if (Hotbar->HasValidSelectedIndex()) return;

	if (GetNetMode() == NM_Client)
	{
		ServerUnEquipWeapon();
		return;
	}

	ABSWeapon* OldWeapon = MainWeapon;
	if (OldWeapon)
	{
		OldWeapon->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		OldWeapon->SetActorHiddenInGame(true);

		OldWeapon->OnUnEquipped();

		OldWeapon->OnWeaponFiring.Unbind();
		OldWeapon->OnWeaponFiringSet.Unbind();
	}

	MainWeapon = nullptr;

	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_MainWeapon(OldWeapon);
		return;
	}

	Hotbar->NotifyEquip();
	SetWeaponVisibility(false);
}

void ABulletSystemCharacter::ServerUnEquipWeapon_Implementation()
{
	UnEquipWeapon();
}

void ABulletSystemCharacter::SetWeaponVisibility(bool bVisible)
{
	WeaponHolder->SetRelativeScale3D(bVisible ? FVector::OneVector : FVector::ZeroVector);

	if (GetWeapon())
	{
		if (bVisible)
		{
			RestoreWeaponVisibility();
		}
		else
		{
			if (!GetWeapon()->IsHidden())
			{
				GetWeapon()->SetActorHiddenInGame(true);
			}
		}
	}
}

void ABulletSystemCharacter::RestoreWeaponVisibility()
{
	WeaponHolder->SetRelativeScale3D(FVector::OneVector);

	if (GetWeapon())
	{
		GetWeapon()->SetActorHiddenInGame(false);
	}
}

void ABulletSystemCharacter::OnActionStarted(bool bDisableAds /*= true*/)
{
	if (bFiring)
	{
		if (IsLocallyControlled())
		{
			ServerSetFiring(false);
		}

		TryFire(false);
	}

	// Parar ADS
}

bool ABulletSystemCharacter::HasActiveAction() const
{
	return bMontagePlaying;
}

bool ABulletSystemCharacter::TryFire(bool bFire)
{
	if (HasActiveAction()) return false;

	if (GetWeapon())
	{
		if (bFire)
		{
			GetWeapon()->OnStartFire();
		}
		else
		{
			GetWeapon()->OnStopFire();
		}
	}

	return true;
}

void ABulletSystemCharacter::OnRep_MainWeapon(ABSWeapon* InOldMainWeapon)
{
	if (InOldMainWeapon)
	{
		InOldMainWeapon->OnWeaponFiring.Unbind();
		InOldMainWeapon->OnWeaponFiringSet.Unbind();

		// Parar reloads?

		// Parar sons?

		OldMainWeapon = InOldMainWeapon;
	}

	if (MainWeapon)
	{
		InitializeWeapon();
	}
	else
	{
		if (ITHotbarItemInterface* OldMainItemInterface = Cast<ITHotbarItemInterface>(InOldMainWeapon))
		{
			OldMainItemInterface->StopEquipAnimation(this, true);
			OldMainItemInterface->StopEquipAnimation(this, false);
			OldMainItemInterface->StopUnEquipAnimation(this, true);
			OldMainItemInterface->StopUnEquipAnimation(this, false);
		}
	}
}

void ABulletSystemCharacter::OnRep_BulletSpawnData()
{
	if (!IsLocallyControlled())
	{
		PerformSpawnBullet(BulletSpawnData.Location, BulletSpawnData.Direction);
	}
}

void ABulletSystemCharacter::OnRep_BulletSpawnDataGauge()
{
	if (!IsLocallyControlled())
	{
		PerformSpawnGauge(BulletSpawnDataGauge.Location, BulletSpawnDataGauge.Directions);
	}
}
