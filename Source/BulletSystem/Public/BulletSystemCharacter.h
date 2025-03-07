// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "THotbarCharacterInterface.h"
#include "BSSharedTypes.h"
#include "BulletSystemCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

class ABSWeapon;
class UTHotbarComponent;
class UBSAnimInstance;
class UBSWeaponSwayComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponAdded, ABSWeapon*, WeaponRef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponRemoved, ABSWeapon*, WeaponRef);

UCLASS(config=Game)
class ABulletSystemCharacter : public ACharacter
	, public ITHotbarCharacterInterface
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BS Character", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BS Character", meta = (AllowPrivateAccess = "true"))
	USceneComponent* WeaponHolder;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BS Character", meta = (AllowPrivateAccess = "true"))
	UTHotbarComponent* Hotbar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BS Character", meta = (AllowPrivateAccess = "true"))
	UBSWeaponSwayComponent* WeaponSway;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ChangeWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (MetaClass = "/Script/BS_Weapon.BSWeapon"))
	TArray<FSoftClassPath> DefaultWeapons;
	
public:
	ABulletSystemCharacter(const FObjectInitializer& ObjectInitializer);

	//~AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	//~AActor
	
	//~APawn
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void OnRep_PlayerState() override;

	virtual void AddControllerPitchInput(float Val) override;
	virtual void AddControllerYawInput(float Val) override;
	//~APawn

	//~ITHotbarCharacterInterface
	virtual void OnItemStartedChange(AActor* Item, AActor* OldItem) override;
	virtual void OnItemChange(AActor* Item, AActor* OldItem) override;
	virtual void OnItemEndedChange(AActor* Item, AActor* OldItem) override;

	virtual void OnItemAdded(AActor* Item) override;
	virtual void OnItemRemoved(AActor* Item) override;

	virtual bool ShouldPlayEquipAnimation() const override;
	virtual bool ShouldPlayUnEquipAnimation() const override;
	//~ITHotbarCharacterInterface

public:
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponAdded OnWeaponAdded;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWeaponRemoved OnWeaponRemoved;

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Fire(const FInputActionValue& Value);

	void ChangeWeapon(const FInputActionValue& Value);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Weapon", ReplicatedUsing = OnRep_MainWeapon)
	TObjectPtr<ABSWeapon> MainWeapon;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<ABSWeapon> OldMainWeapon;

	UPROPERTY(Replicated)
	int32 WeaponCount = INDEX_NONE;

	UPROPERTY(Replicated)
	uint8 bFiring : 1;

	UPROPERTY()
	uint8 bMontagePlaying : 1;

	UPROPERTY()
	UBSAnimInstance* AnimInstance;

	UPROPERTY(ReplicatedUsing = OnRep_BulletSpawnData)
	FBulletSpawnData BulletSpawnData;
	UPROPERTY(ReplicatedUsing = OnRep_BulletSpawnDataGauge)
	FBulletSpawnDataGauge BulletSpawnDataGauge;

	FTimerHandle TimerHandle_WeaponVisibility;

public:
	float LastControllerPitchInput;
	float LastControllerYawInput;

protected:
	virtual void SpawnDefaultWeapons();
	UFUNCTION(Client, Reliable)
	void ClientOnSpawnDefaultWeapons();

	void OnHotbarItemsUpdated();

public:
	virtual void OnWeaponFiringSet(bool bIsFiring);
	virtual void OnWeaponFiring();

	virtual void OnFireCancelled();
	UFUNCTION(Server, Reliable)
	void ServerSetFiring(bool bIsFiring);

	virtual FTransform GetBulletSpawnTransform() const;
	virtual FTransform GetEyeViewPoint() const;

	void FireProjectile();

	virtual void SpawnBullet();
	virtual void SpawnGauge(int32 PelletsCount);

	virtual void PerformSpawnBullet(FVector_NetQuantize100 InLocation, FVector_NetQuantizeNormal InDirection);
	UFUNCTION(Server, Reliable)
	virtual void ServerPerformSpawnBullet(FVector_NetQuantize100 InLocation, FVector_NetQuantizeNormal InDirection);

	virtual void PerformSpawnGauge(FVector_NetQuantize100 InLocation, const TArray<FVector_NetQuantizeNormal>& InDirections);
	UFUNCTION(Server, Reliable)
	virtual void ServerPerformSpawnGauge(FVector_NetQuantize100 InLocation, const TArray<FVector_NetQuantizeNormal>& InDirections);

public:
	virtual void InitializeWeapon();

	virtual void EquipWeapon();
	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon();

	virtual void UnEquipWeapon();
	UFUNCTION(Server, Reliable)
	void ServerUnEquipWeapon();

	void SetWeaponVisibility(bool bVisible);
	void RestoreWeaponVisibility();

	virtual void OnActionStarted(bool bDisableAds = true);
	virtual bool HasActiveAction() const;

public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool TryFire(bool bFire);

protected:
	UFUNCTION()
	virtual void OnRep_MainWeapon(ABSWeapon* InOldMainWeapon);

	UFUNCTION()
	virtual void OnRep_BulletSpawnData();

	UFUNCTION()
	virtual void OnRep_BulletSpawnDataGauge();

public:
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE ABSWeapon* GetWeapon() const { return MainWeapon; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FORCEINLINE bool IsFiring() const { return bFiring; }

	//~Sway
	//~Sway

};

