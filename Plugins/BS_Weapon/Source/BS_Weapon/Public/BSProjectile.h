#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BSSharedTypes.h"
#include "BSProjectile.generated.h"

class AWindDirectionalSource;
class UDamageType;

USTRUCT(BlueprintType)
struct FBulletOnHitData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FHitResult HitResult;

	UPROPERTY()
	FVector_NetQuantize100 Velocity;

	UPROPERTY()
	float Speed;

	FBulletOnHitData()
		: HitResult(ForceInit)
		, Velocity(FVector::ZeroVector)
		, Speed(0.0f)
	{}

	FBulletOnHitData(FHitResult InHitResult, FVector_NetQuantize100 InVelocity, float InSpeed)
		: HitResult(InHitResult)
		, Velocity(InVelocity)
		, Speed(InSpeed)
	{}

	bool IsValid() const
	{
		return HitResult.bBlockingHit && HitResult.GetActor();
	}
};

USTRUCT(BlueprintType)
struct FBulletOnOutData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FHitResult HitResult;

	UPROPERTY()
	FVector_NetQuantize100 Velocity;

	UPROPERTY()
	float Speed;

	UPROPERTY()
	FVector_NetQuantize100 ExitLocation;

	FBulletOnOutData()
		: HitResult(ForceInit)
		, Velocity(FVector::ZeroVector)
		, Speed(0.0f)
		, ExitLocation(FVector_NetQuantize100::ZeroVector)
	{}

	FBulletOnOutData(FHitResult InHitResult, FVector_NetQuantize100 InVelocity, float InSpeed, FVector_NetQuantize100 InExitLocation)
		: HitResult(InHitResult)
		, Velocity(InVelocity)
		, Speed(InSpeed)
		, ExitLocation(InExitLocation)
	{}

	bool IsValid() const
	{
		return HitResult.bBlockingHit && HitResult.GetActor();
	}
};

DECLARE_DELEGATE_TwoParams(FOnBulletHit, class ABSProjectile* /* BulletRef */, const FHitResult& /* HitResult */);
DECLARE_DELEGATE_TwoParams(FOnBulletOut, class ABSProjectile* /* BulletRef */, const FHitResult& /* HitResult */);
DECLARE_DELEGATE_TwoParams(FOnBulletBounce, class ABSProjectile* /* BulletRef */, const FHitResult& /* HitResult */);

UCLASS()
class BS_WEAPON_API ABSProjectile : public AActor
{
	GENERATED_BODY()

	UPROPERTY(Category = "Projectile", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Category = "Projectile", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> Tracer;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (ClampMin = "0.0"))
	float LifeTimeDuration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (ClampMin = "0.0"))
	float BaseDamage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (ExposeOnSpawn = true))
	float Diameter; // projectile diameter

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (ExposeOnSpawn = true))
	float Mass; // projectile mass

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (ExposeOnSpawn = true))
	float MuzzleVelocity; // meters/second 500 is AK-47

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (ExposeOnSpawn = true))
	float Shape; // for calculating drag coefficient

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs", meta = (ExposeOnSpawn = true))
	int32 MaxBounceTimes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs|Debug", meta = (ExposeOnSpawn = true))
	uint8 bDisableDistraction : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs|Debug", meta = (EditCondition = "bShowDebugHitLocation", EditConditionHides, ExposeOnSpawn = true))
	float DebugHitRadius;

public:
	ABSProjectile(const FObjectInitializer& ObjectInitializer);

	//~AActor
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Reset() override;
	virtual void LifeSpanExpired() override;
	//~AActor

public:
	FOnBulletHit OnBulletHit;
	FOnBulletOut OnBulletOut;
	FOnBulletBounce OnBulletBounce;

public:
	/*substep delta time in seconds*/
	UFUNCTION(BlueprintPure)
	virtual float GetSubstepDeltaTime() { return 0.01f; }

	/*Weather to enable substepping/ Fixed update*/
	UFUNCTION(BlueprintPure)
	virtual bool GetEnableSubstepping() { return true; }

	/*Do simulation here. Delta time is mostly fixed, hence results consistent between varying frame rate*/
	virtual void SubstepTick(float DeltaTime);

protected:
	FORCEINLINE FLinearColor LinearColorLerp(FLinearColor A, FLinearColor B, float Alpha) const
	{
		return A + Alpha * (B - A);
	};

	FLinearColor GetTraceColor() const;
	virtual bool IsDebugVisualBulletEnabled() const;
	virtual float GetDebugDuration() const;

protected:
	virtual void CheckForAnyHit();

	virtual void InitializeProjectile();
	virtual void OnHit(const FHitResult& HitResult);
	virtual void OnPostHit(const FHitResult& HitResult);
	FHitResult GetHitResultWithPhysMaterial(const FHitResult& HitResult);

	virtual void DealDamage(const FHitResult& HitResult);
	virtual float CalcDamage(const FHitResult& HitResult, const FVector& ImpactVelocity) const;

	virtual void DestroyOnNextTick();

protected:
	bool DecideWhetherToPenetrate(const FHitResult& HitResult);
	void ComputeExitLocation(const FHitResult& HitResult, FVector& ExitLocation, FVector& ExitNormal, bool& ResultFound);
	float GetNewSpeed(const FHitResult& HitResult, const FVector& ExitLocation);
	bool CanBounce(const FHitResult& HitResult);
	float DecreaseDamageByPercent(float Percent) const;

	void OnPenetrate(const FHitResult& HitResult, const FVector& ExitLocation);
	void OnBounce(const FHitResult& HitResult);

	void ApplyChanges(const FTransform& InSpwanTransform
		, float InMuzzleVelocity
		, float InOriginalSpeed
		, float InBaseDamage
		, float InLifeTimeDuration
		, int32 InBounceTimes);

protected:
	virtual void EventOnHit(const FBulletOnHitData& HitData);
	virtual void CheckFXOnHit(const FHitResult& HitResult);
	virtual void EventBulletOut(const FBulletOnOutData& HitData);
	virtual void EventBounce(const FBulletOnHitData& HitData);

	virtual void SpawnImpactFXHandler(EImpactType ImpactType, const FHitResult& HitResult, const FVector& InVelocity, float InSpeed, const FVector& ExitLocation);

public:
	void OnAddedToPool();
	void ResetForPool();
	void OnRemovedFromPool();

protected:
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Added To Pool"))
	void K2_OnAddedToPool();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Reset For Pool"))
	void K2_ResetForPool();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Removed From Pool"))
	void K2_OnRemovedFromPool();

protected:
	UPROPERTY()
	FVector InitialPosition;

	UPROPERTY()
	FVector_NetQuantize EnvironmentVelocity; // Wind velocity

	UPROPERTY()
	uint8 bUsingRho : 1; // Shall we calculate air density

	UPROPERTY()
	FVector_NetQuantize Velocity; // projectile velocity

	UPROPERTY()
	int32 BounceTimes;

	UPROPERTY()
	float OriginalSpeed;

	UPROPERTY()
	FVector_NetQuantize OriginalLocation;

	float Timespan;
	FVector TracerSize;
	FVector TracerScale;

	float Rho;
	FVector FluidVelocity;
	float DragForce;

	FVector_NetQuantize LastPosition;
	UPROPERTY()
	FVector_NetQuantize NextPosition;

	// Local uses only
	float ActualSpeed;
	float LastPositionTime;
	
	UPROPERTY()
	TObjectPtr<AWindDirectionalSource> WindDirectionalSource;

	UPROPERTY()
	TObjectPtr<AActor> LastHitActor;

	UPROPERTY()
	uint8 bHasHit : 1;

	FVector LastTraceStart;
	FVector LastTraceEnd;

	UPROPERTY()
	uint8 bCanPenetrateSurface : 1;

	UPROPERTY()
	uint8 bCanBounce : 1;

	UPROPERTY()
	FHitResult FXHitResult;

	UPROPERTY()
	FPenetrateData PenetrateData;

	float AirDensity;
	float WaterDensity;
	float FrontalArea;
	float ProjectileCrossSectionalArea;
	float DragCoefficientMultiplier;
	float Cd;
	float Pressure;
	float Temperature;
	float MolarMass;
	float GasConstant;

	float InitialDiameter;
	float InitialMass;
	float InitialMuzzleVelocity;
	float InitialShape;
	int32 InitialMaxBounceTimes;
	float InitialLifeTimeDuration;
	float InitialBaseDamage;
	FVector InitialTracerSize;

	bool bInitialIsSet;

public:
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetActualSpeed() const { return ActualSpeed; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE FVector GetActualVelocity() const { return Velocity; }
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetInitialSpeed() const { return MuzzleVelocity; }

public:
	virtual bool IsLocallyControlled() const;
	virtual bool IsPlayerControlled() const;

public:
	uint8 bUnlistedFromDebugVisual : 1;
	void UnlistedFromDebugVisual();
};
