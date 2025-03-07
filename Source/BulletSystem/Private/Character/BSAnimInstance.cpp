// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BSAnimInstance.h"

#include "BulletSystemCharacter.h"
#include "GameFramework/PlayerController.h"

UBSAnimInstance::UBSAnimInstance()
{
}

void UBSAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ABulletSystemCharacter>(TryGetPawnOwner());
}

void UBSAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!Character) return;

	const FRotator BaseAimRotation = Character->IsLocallyControlled() ? Character->GetControlRotation() : Character->GetBaseAimRotation();

	FRotator AddRot;
#if 0
	if (APlayerController* PC = Character->GetController<APlayerController>())
	{
		AddRot = FRotator(
			Character->LastControllerPitchInput * PC->InputPitchScale_DEPRECATED,
			Character->LastControllerYawInput * PC->InputYawScale_DEPRECATED,
			0.0f);
	}
#endif

	FRotator Delta = (BaseAimRotation + AddRot) - Character->GetActorRotation();
	Delta.Normalize();

	AimRotation = Delta;
}
