// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BSAnimInstance.generated.h"

class ABulletSystemCharacter;

/**
 * 
 */
UCLASS()
class BULLETSYSTEM_API UBSAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UBSAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pose")
	TObjectPtr<UAnimSequence> BasePose;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pose")
	FRotator AimRotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pose")
	FVector PivotOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pose")
	FVector WeaponOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sway")
	FTransform MovingSway;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sway")
	FRotator FinalSwayRot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sway")
	FVector2D FinalSwayLoc;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	ABulletSystemCharacter* Character;
	
};
