#pragma once

#include "BSSharedTypes.generated.h"

#define BS_PRINT_FILE (FString(FPaths::GetCleanFilename(TEXT(__FILE__))))
#define BS_PRINT_FUNC (FString(__FUNCTION__))
#define BS_PRINT_LINE (FString::FromInt(__LINE__))
#define BS_LOGS_LINE (BS_PRINT_FUNC + TEXT(" [") + BS_PRINT_FILE + TEXT(":") + BS_PRINT_LINE + TEXT("]"))


UENUM(BlueprintType)
enum class EProjectileType : uint8
{
	EPT_Cartridge		UMETA(DisplayName = "Cartridge"),
	EPT_Gauge			UMETA(DisplayName = "Gauge")
};

UENUM(BlueprintType)
enum class EImpactType : uint8
{
	EIT_None			UMETA(DisplayName = "None"),
	EIT_Terminate		UMETA(DisplayName = "Terminate"),
	EIT_Ricochet		UMETA(DisplayName = "Ricochet"),
	EIT_Penetrate		UMETA(DisplayName = "Penetrate")
};

USTRUCT(BlueprintType)
struct FBulletSpawnData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector_NetQuantize Location;

	UPROPERTY()
	FVector_NetQuantizeNormal Direction;

	FBulletSpawnData()
		: Location(FVector_NetQuantize::ZeroVector)
		, Direction(FVector_NetQuantizeNormal::ZeroVector)
	{}

	FBulletSpawnData(FVector_NetQuantizeNormal const& InLocation, FVector_NetQuantizeNormal const& InDirection)
		: Location(InLocation)
		, Direction(InDirection)
	{}
};

USTRUCT(BlueprintType)
struct FBulletSpawnDataGauge
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector_NetQuantize Location;

	UPROPERTY()
	TArray<FVector_NetQuantizeNormal> Directions;

	FBulletSpawnDataGauge()
		: Location(FVector_NetQuantize::ZeroVector)
		, Directions({})
	{}

	FBulletSpawnDataGauge(FVector_NetQuantizeNormal const& InLocation, const TArray<FVector_NetQuantizeNormal>& InDirections)
		: Location(InLocation)
		, Directions(InDirections)
	{}
};

USTRUCT(BlueprintType)
struct FPenetrateData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	bool bPenetrated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	bool bSlippery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	float JoulesPerCM;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	float ImpactAngleCoefficient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	float ImpactAngleConstant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	float VelocityCoefficient;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Defaults)
	float VelocityConstant;

	FPenetrateData()
		: bPenetrated(false)
		, bSlippery(true)
		, JoulesPerCM(12000.0f)
		, ImpactAngleCoefficient(-0.0222f)
		, ImpactAngleConstant(1.7f)
		, VelocityCoefficient(0.000125f)
		, VelocityConstant(-5.0f)
	{}
};

USTRUCT(BlueprintType)
struct FWeaponAnimData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TObjectPtr<UAnimSequence> Pose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	FVector WeaponOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TObjectPtr<UAnimMontage> CharDrawMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TObjectPtr<UAnimMontage> WeaponDrawMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TObjectPtr<UAnimMontage> CharUnEquipMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
	TObjectPtr<UAnimMontage> WeaponUnEquipMontage;
};