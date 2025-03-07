#include "BSWeaponSwayComponent.h"

#include "GameFramework/Character.h"

UBSWeaponSwayComponent::UBSWeaponSwayComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UBSWeaponSwayComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = CastChecked<ACharacter>(GetOwner());
}

void UBSWeaponSwayComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwningCharacter) return;
	if (!OwningCharacter->IsLocallyControlled()) return;

	ApplyMovementSway(DeltaTime);
	CalculateSway(DeltaTime);
}

void UBSWeaponSwayComponent::OnLook(const FVector2D& InLookAxisVector)
{
	if (LastMouseDelta.X != 0.0f)
	{
		RotAimSway.Roll += AimSwayData.RotIntensity.X * InLookAxisVector.X;
		RotAimSway.Yaw += AimSwayData.RotIntensity.Z * InLookAxisVector.X;
		LocAimSway.X += AimSwayData.LocIntensity.X * InLookAxisVector.X;
	}

	if (LastMouseDelta.Y != 0.0f)
	{
		RotAimSway.Pitch += AimSwayData.RotIntensity.Y * InLookAxisVector.Y;
		LocAimSway.Y += AimSwayData.LocIntensity.Y * InLookAxisVector.Y;
	}

	LastMouseDelta = InLookAxisVector;
}

void UBSWeaponSwayComponent::ApplyMovementSway(float DeltaSeconds)
{
	const FVector Velocity = OwningCharacter->GetVelocity();
	const FRotator WorldRotation = OwningCharacter->GetActorRotation();

	FVector LocalVelocity = WorldRotation.UnrotateVector(Velocity);
	LocalVelocity.Normalize(1.0f);

	float SpeedRot = MoveSwayData.InterpSpeedRot;
	float SpeedLoc = MoveSwayData.InterpSpeedLoc;
	float Accel = MoveSwayData.Acceleration;

	FVector BaseLoc = MoveSwayData.MovementSwayTarget.GetLocation();
	FRotator BaseRot = MoveSwayData.MovementSwayTarget.Rotator();

	MovementDirection = FMath::VInterpTo(MovementDirection, LastLocalVelocity, DeltaSeconds, Accel);

	MoveSwayRatio = MovementDirection.Size();

	BaseLoc.X *= -MovementDirection.X;
	BaseLoc.Y *= MovementDirection.Y;
	BaseLoc.Z *= MovementDirection.X;

	BaseRot.Pitch *= -MovementDirection.X;
	BaseRot.Roll *= MovementDirection.Y;
	BaseRot.Yaw *= MovementDirection.Y;

	const float AlphaLoc = FMath::Min(DeltaSeconds * SpeedLoc, 1.0f);
	const float AlphaRot = FMath::Min(DeltaSeconds * SpeedRot, 1.0f);

	FVector LerpedLoc = UKismetMathLibrary::VectorSpringInterp(MovingSway.GetLocation(), BaseLoc, MovingSpringLoc, 0.8f, 0.35f, AlphaLoc);

	MovingSway.SetLocation(LerpedLoc);

	FVector Start(MovingSway.Rotator().Pitch, MovingSway.Rotator().Yaw, MovingSway.Rotator().Roll);
	FVector End(BaseRot.Pitch, BaseRot.Yaw, BaseRot.Roll);

	Start = UKismetMathLibrary::VectorSpringInterp(Start, End, MovingSpringRoc, 0.8f, 0.35f, AlphaRot);

	FQuat OutRot = FRotator(Start.X, Start.Y, Start.Z).Quaternion();
	OutRot.Normalize();
	MovingSway.SetRotation(OutRot);

	LastLocalVelocity = LocalVelocity;
}

void UBSWeaponSwayComponent::CalculateSway(float DeltaSeconds)
{
	FRotator& OutRot = FinalSwayRot;
	FVector2D& OutLoc = FinalSwayLoc;

	auto Spring = [&](double& Current, float Target, FFloatSpringState& State, float Stiffness, float Damp, float Delta, float Mass)
	{
		Current = UKismetMathLibrary::FloatSpringInterp(Current, Target, State, Stiffness, Damp, FMath::Min(Delta, 1.f), Mass);
	};

	RotAimSway.Pitch = FMath::Clamp(RotAimSway.Pitch, -AimSwayData.ClampRot.Pitch, AimSwayData.ClampRot.Pitch);
	RotAimSway.Yaw = FMath::Clamp(RotAimSway.Yaw, -AimSwayData.ClampRot.Yaw, AimSwayData.ClampRot.Yaw);
	RotAimSway.Roll = FMath::Clamp(RotAimSway.Roll, -AimSwayData.ClampRot.Roll, AimSwayData.ClampRot.Roll);

	LocAimSway.X = FMath::Clamp(LocAimSway.X, -AimSwayData.ClampLoc.X, AimSwayData.ClampLoc.X);
	LocAimSway.Y = FMath::Clamp(LocAimSway.Y, -AimSwayData.ClampLoc.Y, AimSwayData.ClampLoc.Y);

	Spring(OutRot.Pitch, RotAimSway.Pitch, SwaySpringPitch, AimSwayData.Stiffness.Y, AimSwayData.Damping.Y, AimSwayData.RotAccel.Y * DeltaSeconds, AimSwayData.Mass.Y);
	Spring(OutRot.Yaw, RotAimSway.Yaw, SwaySpringYaw, AimSwayData.Stiffness.Z, AimSwayData.Damping.Z, AimSwayData.RotAccel.Z * DeltaSeconds, AimSwayData.Mass.Z);
	Spring(OutRot.Roll, RotAimSway.Roll, SwaySpringRoll, AimSwayData.Stiffness.X, AimSwayData.Damping.X, AimSwayData.RotAccel.X * DeltaSeconds, AimSwayData.Mass.X);

	AimSwayRatio = (FMath::Abs(OutRot.Pitch) / AimSwayData.ClampRot.Pitch + FMath::Abs(OutRot.Yaw) / AimSwayData.ClampRot.Yaw) / 1.f;
	AimSwayRatio = FMath::Min(AimSwayRatio, 1.f);

	RotAimSway = FMath::RInterpTo(RotAimSway, FRotator::ZeroRotator, DeltaSeconds, AimSwayData.RotDamp);

	OutLoc.X = FMath::FInterpTo(OutLoc.X, LocAimSway.X, DeltaSeconds, AimSwayData.LocAccel.X);
	OutLoc.Y = FMath::FInterpTo(OutLoc.Y, LocAimSway.Y, DeltaSeconds, AimSwayData.LocAccel.Y);

	LocAimSway = FMath::Vector2DInterpTo(LocAimSway, FVector2D::ZeroVector, DeltaSeconds, AimSwayData.LocDamp);
}
