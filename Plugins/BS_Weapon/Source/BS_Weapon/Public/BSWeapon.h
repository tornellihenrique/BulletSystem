#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BSSharedTypes.h"
#include "THotbarItemInterface.h"
#include "BSWeapon.generated.h"

class ABSProjectile;

DECLARE_DELEGATE(FOnWeaponFiring);
DECLARE_DELEGATE_OneParam(FOnWeaponFiringSet, bool /*bIsFiring*/)

UCLASS(Abstract, meta = (DisplayName = "Weapon"))
class BS_WEAPON_API ABSWeapon : public AActor
	, public ITHotbarItemInterface
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	float FireRate;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	EProjectileType ProjectileType;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	TSubclassOf<ABSProjectile> ProjectileClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (EditCondition = "ProjectileType == EProjectileType::EPT_Gauge", EditConditionHides))
	int32 GaugePelletsCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs|Animations", meta = (ExposeOnSpawn = true))
	FWeaponAnimData WeaponAnimData;

public:
	ABSWeapon(const FObjectInitializer& ObjectInitializer);

	//~AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;

	virtual void OnRep_Instigator() override;
	//~AActor

	//~ITHotbarItemInterface
	virtual float PlayEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true, float InPlayRate = 1.0f) override;
	virtual void StopEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true) override;
	virtual bool IsPlayingEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true) override;
	virtual float PlayUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true, float InPlayRate = 1.0f) override;
	virtual void StopUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true) override;
	virtual bool IsPlayingUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true) override;
	//~ITHotbarItemInterface

public:
	FOnWeaponFiring OnWeaponFiring;
	FOnWeaponFiringSet OnWeaponFiringSet;

public:
	virtual void OnStartFire();
	virtual void OnStopFire();

	virtual bool CanFire() const;

	virtual void SetEquippedSlot(int32 InEquippedSlot);

	virtual void OnEquipped(int32 InSlot);
	virtual void OnUnEquipped();

	virtual void OnPickup(ACharacter* InCharacter);
	virtual void OnDrop(ACharacter* InCharacter);

	virtual void OnAdded(ACharacter* InCharacter);
	virtual void OnRemoved(ACharacter* InCharacter);

	UFUNCTION()
	virtual void OnFired();
	UFUNCTION()
	virtual void OnFiring();
	UFUNCTION()
	virtual void OnFiredFX();
	UFUNCTION()
	virtual void OnFiringSet(bool bIsFiring);
	UFUNCTION(Server, Reliable)
	virtual void ServerOnFiringSet(bool bIsFiring);

	void PlayMontage(UAnimMontage* Montage, float PlayRate = 1.f) const;
	void StopMontage(UAnimMontage* Montage = nullptr, float BlendOut = 0.1f) const;

public:
	virtual FTransform GetMuzzleTransform() const;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_EquippedSlot)
	int32 EquippedSlot;

	UPROPERTY()
	int32 BulletFired;

	UPROPERTY(ReplicatedUsing = OnRep_Firing)
	uint8 bFiring : 1;

	UPROPERTY()
	float Timespan;

	float DropSpeed;
	FVector PreviousLocation;
	float PreviousLocationTime;

	FTimerHandle TimerHandle_Fire;

public:
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual int32 GetEquippedSlot() const { return EquippedSlot; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual bool IsFiring() const { return bFiring; }

	FVector GetPivotOffset() const;

protected:
	UFUNCTION(NetMulticast, Reliable)
	virtual void DropWeapon(FVector_NetQuantize InLocation, FVector_NetQuantizeNormal InRotation, FVector_NetQuantizeNormal InDirection);

public:
	virtual bool IsLocallyControlled(APawn* InPawn = nullptr) const;
	virtual bool IsPlayerControlled(APawn* InPawn = nullptr) const;

protected:
	UFUNCTION()
	virtual void OnRep_EquippedSlot();

	UFUNCTION()
	virtual void OnRep_Firing();
};
