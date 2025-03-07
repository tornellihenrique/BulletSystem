#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BSWeaponSwayComponent.generated.h"

USTRUCT(BlueprintType)
struct FMovementSway
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	FTransform MovementSwayTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float InterpSpeedLoc;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float InterpSpeedRot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float Acceleration;

	FMovementSway()
		: MovementSwayTarget(FTransform::Identity)
		, InterpSpeedLoc(0.0f)
		, InterpSpeedRot(0.0f)
		, Acceleration(0.0f)
	{}
};

USTRUCT(BlueprintType)
struct FAimSway
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	FRotator ClampRot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	FVector RotAccel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	FVector Stiffness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	FVector Damping;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	FVector Mass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	FVector RotIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	float RotDamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	FVector2D ClampLoc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Translation")
	FVector2D LocIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Translation")
	FVector2D LocAccel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Translation")
	float LocDamp;

	FAimSway()
		: ClampRot(FRotator::ZeroRotator)
		, RotAccel(FVector::ZeroVector)
		, Stiffness(FVector::ZeroVector)
		, Damping(FVector::ZeroVector)
		, Mass(FVector::ZeroVector)
		, RotIntensity(FVector::ZeroVector)
		, RotDamp(0.0f)
		, ClampLoc(FVector2D::ZeroVector)
		, LocIntensity(FVector2D::ZeroVector)
		, LocAccel(FVector2D::ZeroVector)
		, LocDamp(0.0f)
	{}
};

UCLASS(ClassGroup = (Weapon), meta = (BlueprintSpawnableComponent))
class BS_WEAPON_API UBSWeaponSwayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	FMovementSway MoveSwayData;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Configs")
	FAimSway AimSwayData;

public:
	UBSWeaponSwayComponent();

	//~UActorComponent
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~UActorComponent

public:
	void OnLook(const FVector2D& InLookAxisVector);

	FTransform GetMovingSway() const { return MovingSway; }
	FRotator GetFinalSwayRot() const { return FinalSwayRot; }
	FVector2D GetFinalSwayLoc() const { return FinalSwayLoc; }

protected:
	void ApplyMovementSway(float DeltaSeconds);
	void CalculateSway(float DeltaSeconds);

protected:
	UPROPERTY()
	ACharacter* OwningCharacter;

	FVector2D LastMouseDelta;
	FRotator RotAimSway;
	FVector2D LocAimSway;
	float AimSwayRatio;

	FFloatSpringState SwaySpringPitch;
	FFloatSpringState SwaySpringYaw;
	FFloatSpringState SwaySpringRoll;

	FVector MovementDirection;
	FVector LastLocalVelocity;
	float MoveSwayRatio;
	FVectorSpringState MovingSpringLoc;
	FVectorSpringState MovingSpringRoc;

	FTransform MovingSway;
	FRotator FinalSwayRot;
	FVector2D FinalSwayLoc;
};
