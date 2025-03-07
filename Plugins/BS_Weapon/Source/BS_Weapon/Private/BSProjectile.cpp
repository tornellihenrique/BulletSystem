#include "BSProjectile.h"
#include "BSWorldSettings.h"
#include "BSWeapon.h"
#include "GameFramework/DamageType.h"
#include "Engine/WindDirectionalSource.h"
#include "BSWeaponSettings.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Landscape.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

static int32 DebugVisualBulletEnabled = 0;
FAutoConsoleVariableRef CVar_DebugVisualBulletEnabled(
	TEXT("BS.DebugVisualBulletEnabled"),
	DebugVisualBulletEnabled,
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static float DebugVisualBulletDuration = 5.0f;
FAutoConsoleVariableRef CVar_DebugVisualBulletDuration(
	TEXT("BS.DebugVisualBulletDuration"),
	DebugVisualBulletDuration,
	TEXT("Duration debug should visible e.g 5"),
	ECVF_Cheat);

static int32 BulletNoDamage = 0;
FAutoConsoleVariableRef CVar_BulletNoDamage(
	TEXT("BS.BulletNoDamage"),
	BulletNoDamage,
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static int32 BulletNoPenetrate = 0;
FAutoConsoleVariableRef CVar_BulletNoPenetrate(
	TEXT("BS.BulletNoPenetrate"),
	BulletNoPenetrate,
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static int32 BulletNoRicochet = 0;
FAutoConsoleVariableRef CVar_BulletNoRicochet(
	TEXT("BS.BulletNoRicochet"),
	BulletNoRicochet,
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

static int32 DebugBulletPoolEnabled = 0;
FAutoConsoleVariableRef CVar_DebugBulletPoolEnabled(
	TEXT("BS.DebugBulletPoolEnabled"),
	DebugBulletPoolEnabled,
	TEXT("0: Disable, 1: Enable"),
	ECVF_Cheat);

ABSProjectile::ABSProjectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Diameter(0.02f)
	, Mass(0.1f)
	, MuzzleVelocity(10000.0f)
	, Shape(0.3f)
	, MaxBounceTimes(1)
	, DebugHitRadius(8.0f)
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Tracer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tracer"));
	Tracer->SetupAttachment(Root);
	Tracer->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Tracer->SetCollisionResponseToAllChannels(ECR_Ignore);
	Tracer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tracer->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Tracer->SetRelativeScale3D(FVector(4.0f, 2.0f, 2.0f));
	Tracer->CastShadow = false;
	Tracer->AlwaysLoadOnClient = true;
	Tracer->AlwaysLoadOnServer = true;

#if WITH_EDITORONLY_DATA
	// bListedInSceneOutliner = false;
#endif

	PrimaryActorTick.bCanEverTick = true;

	bReplicates = false;
	bAlwaysRelevant = false;
	SetReplicateMovement(false);
}

void ABSProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	const ABSProjectile* DefaultClass = GetDefault<ABSProjectile>(GetClass());
	if (DefaultClass && DefaultClass->Tracer)
	{
		if (DefaultClass->Tracer)
		{
			TracerSize = DefaultClass->Tracer->GetRelativeScale3D();
		}

		if (!bInitialIsSet)
		{
			InitialDiameter = DefaultClass->Diameter;
			InitialMass = DefaultClass->Mass;
			InitialMuzzleVelocity = DefaultClass->MuzzleVelocity;
			InitialShape = DefaultClass->Shape;
			InitialMaxBounceTimes = DefaultClass->MaxBounceTimes;
			InitialLifeTimeDuration = DefaultClass->LifeTimeDuration;
			InitialBaseDamage = DefaultClass->BaseDamage;
			InitialTracerSize = DefaultClass->Tracer->GetRelativeScale3D();

			bInitialIsSet = true;
		}
	}

	AirDensity = 1.225f; // air density at sea level
	WaterDensity = 250.f; // custom water density (997.f is causing Inf values)
	FrontalArea = PI * (Diameter / 2.0f) * (Diameter / 2.0f); // frontal area of the projectile;
	ProjectileCrossSectionalArea = PI * (Diameter / 2.0f) * (Diameter / 2.0f); // cross-sectional area of the projectile
	DragCoefficientMultiplier = Shape; // drag coefficient multiplier based on projectile shape
	Cd = 0.5f * (FrontalArea / ProjectileCrossSectionalArea) * DragCoefficientMultiplier; // calculate drag coefficient
	Pressure = 101325.0f; // atmospheric pressure at sea level in Pascals
	Temperature = 293.15f; // atmospheric temperature at sea level in Kelvin
	MolarMass = 0.0289644f; // molar mass of dry air in kg/mol
	GasConstant = 8.31447f; // universal gas constant in J/mol.K
}

void ABSProjectile::BeginPlay()
{
	Super::BeginPlay();

	InitializeProjectile();
}

void ABSProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetEnableSubstepping())
	{
		float SubstepDeltaTime = GetSubstepDeltaTime();
		ensure(SubstepDeltaTime > 0.0f);

		int NumberOfSubsteps = FMath::Max<int>((int)(DeltaTime / SubstepDeltaTime), 1);
		const float RemainderDeltaTime = DeltaTime - (NumberOfSubsteps * SubstepDeltaTime);
		
		if (RemainderDeltaTime > 0.0f) NumberOfSubsteps++;

		for (int i = 0; i < NumberOfSubsteps; i++)
		{
			const float SubstepDt = (i == NumberOfSubsteps - 1 && RemainderDeltaTime > 0.0f) ? RemainderDeltaTime : SubstepDeltaTime;
			SubstepTick(SubstepDt);
		}
	}
	else
	{
		SubstepTick(DeltaTime);
	}
}

void ABSProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);

	Super::EndPlay(EndPlayReason);
}

void ABSProjectile::Reset()
{
	Super::Reset();

	ABSProjectile* MutableDefaultClass = GetMutableDefault<ABSProjectile>(GetClass());

	if (MutableDefaultClass)
	{
		Diameter = MutableDefaultClass->Diameter;
		Mass = MutableDefaultClass->Mass;
		MuzzleVelocity = MutableDefaultClass->MuzzleVelocity;
		Shape = MutableDefaultClass->Shape;
		MaxBounceTimes = MutableDefaultClass->MaxBounceTimes;
		LifeTimeDuration = MutableDefaultClass->LifeTimeDuration;
		BaseDamage = MutableDefaultClass->BaseDamage;
		if (MutableDefaultClass->Tracer)
		{
			TracerSize = MutableDefaultClass->Tracer->GetRelativeScale3D();
		}
		Tracer->SetRelativeScale3D(TracerSize);
		EnvironmentVelocity = FVector_NetQuantize::ZeroVector;
		bUsingRho = false;
		Velocity = FVector_NetQuantize::ZeroVector;
		BounceTimes = 0;
		OriginalSpeed = 0.0f;
		OriginalLocation = FVector_NetQuantize::ZeroVector;
		Timespan = 0.0f;
		TracerScale = FVector::ZeroVector;
		Rho = 0.0f;
		FluidVelocity = FVector::ZeroVector;
		DragForce = 0.0f;
		LastPosition = FVector_NetQuantize::ZeroVector;
		NextPosition = FVector_NetQuantize::ZeroVector;
		ActualSpeed = 0.0f;
		LastPositionTime = 0.0f;
		// WindDirectionalSource = nullptr;
		LastHitActor = nullptr;
		bHasHit = false;
		LastTraceStart = FVector::ZeroVector;
		LastTraceEnd = FVector::ZeroVector;
		bCanPenetrateSurface = false;
		bCanBounce = false;
		FXHitResult = FHitResult(ForceInit);
		PenetrateData = FPenetrateData();
		bUnlistedFromDebugVisual = false;
	}
}

void ABSProjectile::LifeSpanExpired()
{
	if (ABSWorldSettings* WS = Cast<ABSWorldSettings>(GetWorld()->GetWorldSettings()))
	{
		const FString& ProjectilBlueprintClassName = FPaths::GetBaseFilename(GetClass()->GetPathName());
		
		// if (WS->ArrayPoolExists(*ProjectilBlueprintClassName))
		{
			OnAddedToPool();
			WS->AddArrayPoolActor(*ProjectilBlueprintClassName, this);

			return;
		}
	}

	Super::LifeSpanExpired();
}

void ABSProjectile::SubstepTick(float DeltaTime)
{
	if (bHasHit) return;

	Timespan += DeltaTime;
	if (GetNetMode() != NM_DedicatedServer)
	{
		float ScaleRatio = 1.0f;
		const float Distance = FVector::Distance(OriginalLocation, LastPosition) / 100.0f;
		const float ScaleXRatio = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 500.0f), FVector2D(0.1f, 1.0f), ActualSpeed);
		if (APlayerController* PC = Cast<APlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			if (AActor* PlayerViewTarget = PC->GetViewTarget())
			{
				const float DistanceToPawn = FVector::Distance(PlayerViewTarget->GetActorLocation(), LastPosition) / 100.0f;
				ScaleRatio = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1000.0f), FVector2D(1.0f, 50.0f), DistanceToPawn);

#if 0
				if (ITCharacterInterface* CharacterInterface = Cast<ITCharacterInterface>(PlayerViewTarget))
				{
					float FpFOV = 90.f;
					static const FString& ConfigSection = TEXT("BHCharacter");
					GConfig->GetFloat(*ConfigSection, TEXT("FpFOV"), FpFOV, GGameIni);

					const ECameraState CameraState = CharacterInterface->GetCameraState();
					const float CameraZoom = CharacterInterface->GetZoom();
					const float TargetScaleRatio = CameraState != ECameraState::Normal ? FMath::GetMappedRangeValueClamped(FVector2D(FpFOV / 1.8f, FpFOV / 15.0f), FVector2D(50.0f, 10.0f), CameraZoom) : 50.0f;

					ScaleRatio = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1000.0f), FVector2D(1.0f, TargetScaleRatio), DistanceToPawn);
				}

				if (SFXComponent && ::IsValid(PlayerViewTarget))
				{
					const FVector& From = FRotationMatrix(Velocity.ToOrientationRotator()).GetScaledAxis(EAxis::X);
					const FVector& To = (PlayerViewTarget->GetActorLocation() - LastPosition);
					const float DotV = FVector::DotProduct(From, To);

					bool bIsSonicBoom = LastHitActor == nullptr;
					SFXComponent->Play_InAir(DotV, From, Distance, ActualSpeed, bIsSonicBoom);
				}
#endif
			}
		}

		const FVector& TargetNewScale = FVector(TracerSize.X * ScaleXRatio, TracerSize.Y * ScaleRatio, TracerSize.Z * ScaleRatio);
		TracerScale = FMath::VInterpTo(TracerScale, TargetNewScale, DeltaTime, 32.0f);

		if (Tracer)
		{
			Tracer->SetRelativeScale3D(TracerScale);
		}
	}

	// get wind/water velocity
	EnvironmentVelocity = FVector::ZeroVector;
	if (!bDisableDistraction && WindDirectionalSource != nullptr)
	{
		if (UWindDirectionalSourceComponent* WindDirectionalSourceComponent = WindDirectionalSource->GetComponent())
		{
			FWindData WindData;
			float Weight;
			WindDirectionalSourceComponent->GetWindParameters(LastPosition, WindData, Weight);

			EnvironmentVelocity = (WindData.Direction * WindData.Speed) * Weight;
		}
	}

	// calculate air density - (Pitch=-90.000000,Yaw=0.000000,Roll=0.000000)
	bUsingRho = LastHitActor != nullptr;
	Rho = bUsingRho ? (Pressure * MolarMass) / (GasConstant * Temperature) : AirDensity;

	// calculate air velocity and relative velocity
	FluidVelocity = Velocity - EnvironmentVelocity;
	const float RelativeVelocity = FluidVelocity.Size();

	// calculate drag force and acceleration due to air/water resistance
	DragForce = 0.5f * Cd * Rho * FrontalArea * RelativeVelocity * RelativeVelocity;
	const FVector DragAcceleration = -1.0 * DragForce * FluidVelocity.GetSafeNormal() / Mass;

	// calculate acceleration due to gravity
	const FVector GravityAcceleration(0.0, 0.0, -9.8f);

	// calculate total acceleration
	const FVector Acceleration = DragAcceleration + GravityAcceleration;

	// update velocity
	Velocity += Acceleration * DeltaTime;

	// Calculate Velocity
	const FVector CurrentLocation = GetActorLocation();
	const FVector CurrentVelocity = (CurrentLocation - LastPosition) / (DeltaTime - LastPositionTime);
	float DistanceTraveled = FVector::Distance(LastPosition, CurrentLocation) / 100.0f;
	ActualSpeed = DistanceTraveled / DeltaTime;

	// Store last position
	LastPosition = CurrentLocation;

	// Set to current frame
	LastPositionTime = DeltaTime;

	// Assign next position
	NextPosition += Velocity * DeltaTime * 100.0f; // velocity m/s -> cm/s && dx = v*dt;

	// Update location and rotation
	SetActorLocationAndRotation(NextPosition, Velocity.ToOrientationRotator());

	// Check for any hit
	CheckForAnyHit();
}

FLinearColor ABSProjectile::GetTraceColor() const
{
	const float ImpactPower3D = Velocity.SizeSquared() / FMath::Square(OriginalSpeed);
	const float SmoothedPower = FMath::SmoothStep(0.0f, 1.0f, ImpactPower3D);

	const FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	const FLinearColor Color0 = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
	const FLinearColor Color1 = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);

	const FLinearColor Color2 = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);
	const FLinearColor Color3 = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	const FLinearColor InterpColor = LinearColorLerp(Color, Color0, SmoothedPower);
	const FLinearColor InterpColor01 = LinearColorLerp(Color0, Color1, SmoothedPower);
	const FLinearColor InterpColor12 = LinearColorLerp(Color1, Color2, SmoothedPower);
	const FLinearColor InterpColor23 = LinearColorLerp(Color2, Color3, SmoothedPower);
	const FLinearColor InterpColor012 = LinearColorLerp(InterpColor01, InterpColor12, SmoothedPower);
	const FLinearColor InterpColor123 = LinearColorLerp(InterpColor12, InterpColor23, SmoothedPower);

	const FLinearColor SmoothedColor = LinearColorLerp(InterpColor012, InterpColor123, SmoothedPower);

	return SmoothedColor;
}

bool ABSProjectile::IsDebugVisualBulletEnabled() const
{
	return !bUnlistedFromDebugVisual && DebugVisualBulletEnabled == 1;
}

float ABSProjectile::GetDebugDuration() const
{
	return DebugVisualBulletDuration;
}

void ABSProjectile::CheckForAnyHit()
{
	FCollisionQueryParams CollisionParams;
	CollisionParams.bReturnPhysicalMaterial = false;
	CollisionParams.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;
	CollisionParams.bTraceComplex = true;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.AddIgnoredActor(GetOwner());
	CollisionParams.AddIgnoredActor(GetInstigator());
	if (LastHitActor)
	{
		CollisionParams.AddIgnoredActor(LastHitActor);
	}

	bool bIsHit = false;
	const FVector& ForwardVector = FRotationMatrix(Velocity.ToOrientationRotator()).GetScaledAxis(EAxis::X);
	FVector TraceStart = LastPosition - (ForwardVector * 1.0f);
	FVector TraceEnd = NextPosition;

	FHitResult Hit(ForceInit);
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, NextPosition, ECC_Visibility, CollisionParams))
	{
		if (Hit.bBlockingHit && Hit.GetActor())
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				LastHitActor = nullptr;

				bIsHit = true;
				TraceEnd = Hit.Location;
				LastHitActor = HitActor;

				if (IsDebugVisualBulletEnabled())
				{
					DrawDebugSphere(GetWorld(), TraceEnd, DebugHitRadius, 10, FLinearColor(0.f, 1.f, 0.f, 1.f).ToFColor(true), false, DebugVisualBulletDuration);
				}
			}
		}
	}

	if (IsDebugVisualBulletEnabled())
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, GetTraceColor().ToFColor(true), false, DebugVisualBulletDuration);
	}

	if (bIsHit)
	{
		FXHitResult = GetHitResultWithPhysMaterial(Hit);
		bHasHit = true;
		OnHit(Hit);
	}

	LastTraceStart = TraceStart;
	LastTraceEnd = TraceEnd;
}

void ABSProjectile::InitializeProjectile()
{
	static IConsoleVariable* CVarDebugVisualBulletEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("BS.DebugVisualBulletEnabled"));
	if (CVarDebugVisualBulletEnabled)
	{
		int32 CVarDebugVisualBulletEnabledValue = CVarDebugVisualBulletEnabled->GetInt();
		if (CVarDebugVisualBulletEnabledValue > 1)
		{
			const FString CVar_DebugVisualBulletEnabledStr = FString::Printf(TEXT("BS.DebugVisualBulletEnabled 1"));
			GEngine->Exec(GetWorld(), *CVar_DebugVisualBulletEnabledStr);
		}
	}

	Velocity = GetActorForwardVector() * MuzzleVelocity;
	NextPosition = LastPosition = InitialPosition = GetActorLocation();

	OriginalLocation = NextPosition;
	if (OriginalSpeed <= 0.0f)
	{
		OriginalSpeed = MuzzleVelocity;
	}

	if (LifeTimeDuration <= 0.0f)
	{
		LifeTimeDuration = 0.001f;
	}

	if (!LastHitActor && IsDebugVisualBulletEnabled())
	{
		DrawDebugPoint(GetWorld(), InitialPosition, 10.0f, FLinearColor(1.f, 0.f, 0.f, 1.f).ToFColor(true), false, 5.0f);
	}

	SetLifeSpan(LifeTimeDuration);

	if (GetNetMode() != NM_DedicatedServer)
	{
		if (ABSWeapon* WeaponRef = GetOwner<ABSWeapon>())
		{
			/*WeaponRef->ListenToBulletEvents(this);*/
		}
	}

	if (!WindDirectionalSource)
	{
		for (TActorIterator<AWindDirectionalSource> ActorItr(GetWorld(), AWindDirectionalSource::StaticClass()); ActorItr; ++ActorItr)
		{
			if (AWindDirectionalSource* Wind = *ActorItr)
			{
				WindDirectionalSource = Wind;
				break;
			}
		}
	}

	SetActorTickEnabled(true);
}

void ABSProjectile::OnHit(const FHitResult& HitResult)
{
	if (!HitResult.bBlockingHit) return;

	// 1. DealDamage if possible
	if (GetInstigatorController())
	{
		const float Distance = (InitialPosition - HitResult.Location).Size() / 100.0f;
		if (Distance >= 0.15f && HitResult.GetActor() && HitResult.GetActor()->CanBeDamaged()) // 0.15cm
		{
			DealDamage(HitResult);
		}
	}

	// 2. Process Post Hit
	OnPostHit(HitResult);
}

void ABSProjectile::OnPostHit(const FHitResult& HitResult)
{
	FHitResult ActualHitResult = FXHitResult;
	ActualHitResult.Location = HitResult.Location;
	ActualHitResult.Normal = HitResult.Normal;
	ActualHitResult.ImpactPoint = HitResult.ImpactPoint;
	ActualHitResult.ImpactNormal = HitResult.ImpactNormal;

	// 1. Check damage-able
	if (HitResult.GetActor() && HitResult.GetActor()->IsA<APawn>() && !HitResult.GetActor()->CanBeDamaged())
	{
		DestroyOnNextTick();

		return;
	}

	// 2. Check for surface
	if (!ActualHitResult.PhysMaterial.Get())
	{
		EventOnHit(FBulletOnHitData(FXHitResult, Velocity, ActualSpeed));
		DestroyOnNextTick();

		return;
	}

	EPhysicalSurface SurfaceType = ActualHitResult.PhysMaterial.Get()->SurfaceType;
	PenetrateData = UBSWeaponSettings::Get()->PenetrateSurfaceTypes[SurfaceType];
	bCanPenetrateSurface = PenetrateData.bPenetrated;
	bCanBounce = PenetrateData.bSlippery;

	// 3. Ceck for penetrate and return
	if (BulletNoPenetrate == 0 && bCanPenetrateSurface && DecideWhetherToPenetrate(ActualHitResult))
	{
		FVector ExitLocation = FVector::ZeroVector;
		FVector ExitNormal = FVector::ZeroVector;
		bool ResultFound = false;
		ComputeExitLocation(ActualHitResult, ExitLocation, ExitNormal, ResultFound);

		if (ResultFound)
		{
			if (IsDebugVisualBulletEnabled())
			{
				DrawDebugSphere(GetWorld(), ActualHitResult.Location, DebugHitRadius, 10, FLinearColor(0.f, 1.f, 0.f, 1.f).ToFColor(true), false, DebugVisualBulletDuration);
				DrawDebugSphere(GetWorld(), ExitLocation, DebugHitRadius, 10, FLinearColor(0.f, 1.f, 0.f, 1.f).ToFColor(true), false, DebugVisualBulletDuration);
			}

			OnPenetrate(ActualHitResult, ExitLocation);

			return;
		}
	}

	// 4. Check for ricochet and return
	if (BulletNoRicochet == 0 && bCanBounce && CanBounce(ActualHitResult))
	{
		OnBounce(ActualHitResult);

		return;
	}

	// 5. Broadcast hit & Destroy otherrwise
	EventOnHit(FBulletOnHitData(FXHitResult, Velocity, ActualSpeed));
	DestroyOnNextTick();
}

FHitResult ABSProjectile::GetHitResultWithPhysMaterial(const FHitResult& HitResult)
{
	FHitResult ActualHitResult = HitResult;
	EPhysicalSurface SurfaceType = EPhysicalSurface::SurfaceType_Default;

	// Check for complex phys mat (material physic default)
	if (UPhysicalMaterial* PhysMaterial = HitResult.PhysMaterial.Get())
	{
		SurfaceType = PhysMaterial->SurfaceType;
	}

	constexpr float Distance = 10.0f;
	FCollisionQueryParams CollisionParams;
	CollisionParams.bReturnPhysicalMaterial = true;
	CollisionParams.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;
	CollisionParams.bTraceComplex = false;
	CollisionParams.AddIgnoredActor(this);
	CollisionParams.AddIgnoredActor(GetOwner());
	CollisionParams.AddIgnoredActor(GetInstigator());

	const FVector& ForwardVector = FRotationMatrix(Velocity.ToOrientationRotator()).GetScaledAxis(EAxis::X);
	const FVector& Start = HitResult.Location - (ForwardVector * Distance);
	const FVector& End = HitResult.Location + (ForwardVector * Distance);

	FHitResult Hit(ForceInit);
	bool bIsHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionParams);
	if (bIsHit && HitResult.GetActor() == Hit.GetActor())
	{
		if (UPhysicalMaterial* PhysicalMaterial = Hit.PhysMaterial.Get())
		{
			ActualHitResult.PhysMaterial = Hit.PhysMaterial;
		}

		if (IsDebugVisualBulletEnabled())
		{
			DrawDebugSphere(GetWorld(), Hit.Location, DebugHitRadius, 10, FLinearColor(0.f, 1.f, 0.f, 1.f).ToFColor(true), false, DebugVisualBulletDuration);
		}

		ActualHitResult.Location = Hit.Location;
		ActualHitResult.Normal = Hit.Normal;
		ActualHitResult.ImpactPoint = Hit.ImpactPoint;
		ActualHitResult.ImpactNormal = Hit.ImpactNormal;
	}

	return ActualHitResult;
}

void ABSProjectile::DealDamage(const FHitResult& HitResult)
{
	if (BulletNoDamage == 1) return;
	if (!GetInstigatorController()) return;

	if (AActor* HitActor = HitResult.GetActor())
	{
		if (!HitActor->CanBeDamaged() || HitActor->GetClass() == GetClass()) return;

		FPointDamageEvent DamageEvent;
		DamageEvent.DamageTypeClass = UDamageType::StaticClass();
		if (DamageType)
		{
			DamageEvent.DamageTypeClass = DamageType;
		}
		DamageEvent.HitInfo = HitResult;
		constexpr float Tolerance = 0.0001f;
		DamageEvent.ShotDirection = Velocity.GetSafeNormal(Tolerance);
		DamageEvent.Damage = CalcDamage(HitResult, Velocity);

#if 0
		if (APawn* HitPawn = Cast<APawn>(HitActor))
		{
			if (AController* OwningPlayer = Cast<AController>(GetInstigatorController()))
			{
				if (OwningPlayer->IsLocalController() && OwningPlayer->IsPlayerController())
				{
					if (UTTimeSyncComponent* TimeSyncComponent = OwningPlayer->GetComponentByClass<UTTimeSyncComponent>())
					{
						if (TimeSyncComponent->RequestHit(DamageEvent, GetOwner(), HitActor, HitResult.Location))
						{
							if (IsDebugVisualBulletEnabled())
							{
								DrawDebugLine(GetWorld(), HitResult.Location - (DamageEvent.ShotDirection * 100.0f), HitResult.Location, FColor::Black, false, DebugVisualBulletDuration);
							}

							return;
						}
					}
				}
			}
		}
#endif

		// Apply force to hit component
		if (HitResult.GetComponent())
		{
			if (HitResult.GetComponent()->IsSimulatingPhysics())
			{
				HitResult.GetComponent()->AddImpulse(Velocity * DamageEvent.Damage * 0.002f, HitResult.BoneName);
			}

			if (UGeometryCollectionComponent* GeometryCollectionComponent = Cast<UGeometryCollectionComponent>(HitResult.GetComponent()))
			{
				// GeometryCollectionComponent->ApplyExternalStrain(HitResult.Item, HitResult.Location, 1600.0f, 0, 1.0f, 5000.0f);
				GeometryCollectionComponent->ApplyExternalStrain(HitResult.Item, HitResult.Location, 100.0f, 0, 1.0f, DamageEvent.Damage);
			}
		}

		HitActor->TakeDamage(DamageEvent.Damage, DamageEvent, GetInstigatorController(), GetOwner());
	}
}

float ABSProjectile::CalcDamage(const FHitResult& HitResult, const FVector& ImpactVelocity) const
{
	float TargetBaseDamage = FMath::Abs(BaseDamage);
	TargetBaseDamage = FMath::Abs(TargetBaseDamage);

	if (TargetBaseDamage == 0.0f) return 0.0f;

	return ImpactVelocity.SizeSquared() / FMath::Square(OriginalSpeed) * TargetBaseDamage;
}

void ABSProjectile::DestroyOnNextTick()
{
	GetWorldTimerManager().SetTimerForNextTick([this]() {
		SetLifeSpan(0.001f);
	});
}

bool ABSProjectile::DecideWhetherToPenetrate(const FHitResult& HitResult)
{
	if (BounceTimes > 0 || !bCanPenetrateSurface) return false;
	if (HitResult.GetActor() && HitResult.GetActor()->IsA<ALandscape>()) return false;

	// Cache frequently accessed data
	constexpr float Tolerance = 0.0001f;
	const FVector& VelocitySafe = Velocity.GetSafeNormal(Tolerance);
	const FVector& ImpactNormalSafe = (HitResult.ImpactNormal * -1.0f).GetSafeNormal(Tolerance);

	// Use fast alternative calculations and early return
	const float DotProduct = FVector::DotProduct(VelocitySafe, ImpactNormalSafe);
	const float ImpactAngle = PenetrateData.ImpactAngleCoefficient * FMath::RadiansToDegrees(FMath::Acos(DotProduct)) + PenetrateData.ImpactAngleConstant;
	const float ImpactVelocity = PenetrateData.VelocityCoefficient * Velocity.Size() + PenetrateData.VelocityConstant;

	// UE_LOG(LogBullet, Warning, TEXT("ImpactAngle: %f - ImpactVelocity: %f"), ImpactAngle, ImpactVelocity);

	return ImpactAngle * ImpactVelocity >= 0.0f;
}

void ABSProjectile::ComputeExitLocation(const FHitResult& HitResult, FVector& ExitLocation, FVector& ExitNormal, bool& ResultFound)
{
	if (!HitResult.GetActor()) return;
	UWorld* World = GetWorld();
	if (!World) return;

	static const FName LineTraceMultiName(TEXT("BulletLineTraceMultiForObjects"));
	FCollisionQueryParams Params(LineTraceMultiName, SCENE_QUERY_STAT_ONLY(BulletTraceUtils), true);
	Params.bTraceComplex = false;
	Params.bReturnPhysicalMaterial = false;
	Params.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Vehicle);
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_Destructible);
	ObjectParams.AddObjectTypesToQuery(ECollisionChannel::ECC_GameTraceChannel10); // Vehicle Coillision

	FVector ProliminaryExitLocation = FVector::ZeroVector;
	if (ObjectParams.IsValid())
	{
		constexpr float Tolerance = 0.0001f;
		constexpr float Range = 500.f;
		const FVector& End = HitResult.Location;
		const FVector& Start = (Velocity.GetSafeNormal(Tolerance) * Range) + End;

		TArray<FHitResult> OutHits;
		if (World->LineTraceMultiByObjectType(OutHits, Start, End, ObjectParams, Params))
		{
			FCollisionQueryParams TmpParams(LineTraceMultiName, SCENE_QUERY_STAT_ONLY(BulletTraceUtils), true);
			TmpParams.bTraceComplex = true;
			TmpParams.bReturnPhysicalMaterial = false;
			TmpParams.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;

			for (const FHitResult& Hit : OutHits)
			{
				if (!Hit.bBlockingHit || !Hit.GetActor()) continue;
				if (Hit.GetActor() != HitResult.GetActor() || Hit.GetActor() == this || Hit.GetActor()->IsA<ALandscape>()) continue;

				ProliminaryExitLocation = Hit.Location;

				const FVector& TmpEnd = ProliminaryExitLocation + Velocity.GetSafeNormal(Tolerance);

				TmpParams.ClearIgnoredSourceObjects();
				TmpParams.AddIgnoredActor(Hit.GetActor());

				FHitResult TmpHit(ForceInit);
				if (!World->LineTraceSingleByChannel(TmpHit, ProliminaryExitLocation, TmpEnd, ECC_Visibility, TmpParams))
				{
					ExitLocation = ProliminaryExitLocation + (Velocity.GetSafeNormal(Tolerance) * 1.0f);
					ExitNormal = Hit.ImpactNormal;
					ResultFound = true;

					break;
				}
			}
		}
	}
}

float ABSProjectile::GetNewSpeed(const FHitResult& HitResult, const FVector& ExitLocation)
{
	// Cache frequently accessed data
	const FVector& ImpactPoint = HitResult.ImpactPoint;

	// Simplify calculations
	constexpr float InHalf = 0.5f;
	const float Energy = InHalf * DragForce * FMath::Square(Velocity.Size2D()); // E = 0.5mv^2
	const float EnergySubtractor = (ExitLocation - ImpactPoint).Size() * (PenetrateData.JoulesPerCM);
	const float TreagetEnergy = Energy - EnergySubtractor;

	return FMath::Sqrt(TreagetEnergy / (InHalf * DragForce)); // v = sqrt(E/0.5mv)
}

bool ABSProjectile::CanBounce(const FHitResult& HitResult)
{
	if (BounceTimes >= MaxBounceTimes || !bCanBounce) return false;

	return Velocity.SizeSquared() / FMath::Square(MuzzleVelocity) >= 0.1f;
}

float ABSProjectile::DecreaseDamageByPercent(float Percent) const
{
	return BaseDamage * (1.0 - Percent / 100.0f);
}

void ABSProjectile::OnPenetrate(const FHitResult& HitResult, const FVector& ExitLocation)
{
	const float BaseDamageResult = DecreaseDamageByPercent(10.0f);
	const float NewSpeed = GetNewSpeed(HitResult, ExitLocation) * 0.5f;
	const float TargetMuzzleVelocity = FMath::Clamp(NewSpeed, -1.0f, MuzzleVelocity);
	const float TargetBaseDamage = (TargetMuzzleVelocity / MuzzleVelocity) * BaseDamageResult;
	const FTransform TargetTransform = FTransform(GetActorRotation().Quaternion(), ExitLocation, FVector::OneVector);

	if (TargetMuzzleVelocity > 0.0f)
	{
		MuzzleVelocity = TargetMuzzleVelocity;
		OriginalSpeed = MuzzleVelocity;
		BaseDamage = TargetBaseDamage;
		LifeTimeDuration -= 2.0f;

		FHitResult TmpHitResult = HitResult;
		TmpHitResult.Location = FXHitResult.Location;
		TmpHitResult.Normal = FXHitResult.Normal;
		TmpHitResult.ImpactPoint = FXHitResult.ImpactPoint;
		TmpHitResult.ImpactNormal = FXHitResult.ImpactNormal;

		EventBulletOut(FBulletOnOutData(TmpHitResult, Velocity, ActualSpeed, ExitLocation));
		ApplyChanges(TargetTransform, TargetMuzzleVelocity, MuzzleVelocity, TargetBaseDamage, LifeTimeDuration, BounceTimes);

		return;
	}

	DestroyOnNextTick();
}

void ABSProjectile::OnBounce(const FHitResult& HitResult)
{
	const float BaseDamageResult = DecreaseDamageByPercent(5.0f);
	const float TargetMuzzleVelocity = Velocity.SizeSquared() / MuzzleVelocity;
	const float TargetBaseDamage = (TargetMuzzleVelocity / MuzzleVelocity) * BaseDamageResult;
	const int32 TargetBounceTimes = BounceTimes + 1;

	const FVector& Normal = (NextPosition - HitResult.ImpactPoint).GetSafeNormal();
	const FVector& ImpactNormal = HitResult.ImpactNormal * (FVector::DotProduct(Normal, HitResult.ImpactNormal) * 2.0f);
	const FVector& Direction = Normal - ImpactNormal;

	const FRotator& TargetRotation = Direction.ToOrientationRotator();
	const FVector& TargetLocation = HitResult.Location + TargetRotation.Vector();
	const FTransform TargetTransform = FTransform(TargetRotation.Quaternion(), TargetLocation, FVector::OneVector);

	FHitResult TmpHitResult = HitResult;
	TmpHitResult.Location = FXHitResult.Location;
	TmpHitResult.Normal = FXHitResult.Normal;
	TmpHitResult.ImpactPoint = FXHitResult.ImpactPoint;
	TmpHitResult.ImpactNormal = FXHitResult.ImpactNormal;

	LifeTimeDuration -= 2.0f;

	EventBounce(FBulletOnHitData(TmpHitResult, Velocity, ActualSpeed));
	ApplyChanges(TargetTransform, TargetMuzzleVelocity, MuzzleVelocity, TargetBaseDamage, LifeTimeDuration, TargetBounceTimes);
}

void ABSProjectile::ApplyChanges(const FTransform& InSpwanTransform, float InMuzzleVelocity, float InOriginalSpeed, float InBaseDamage, float InLifeTimeDuration, int32 InBounceTimes)
{
	MuzzleVelocity = InMuzzleVelocity;
	OriginalSpeed = InOriginalSpeed;
	BaseDamage = InBaseDamage;
	LifeTimeDuration = InLifeTimeDuration;
	BounceTimes = InBounceTimes;

	const FVector TargetLocation = InSpwanTransform.GetLocation();
	const FVector TargetRotation = InSpwanTransform.GetRotation().Rotator().GetNormalized().Vector();

	SetActorTickEnabled(false);
	SetActorLocationAndRotation(TargetLocation, TargetRotation.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
	InitializeProjectile();

	bHasHit = false;
}

void ABSProjectile::EventOnHit(const FBulletOnHitData& HitData)
{
	if (GetNetMode() == NM_DedicatedServer) return;

	SpawnImpactFXHandler(EImpactType::EIT_Terminate, HitData.HitResult, HitData.Velocity, HitData.Speed, FVector::ZeroVector);
	OnBulletHit.ExecuteIfBound(this, HitData.HitResult);

	const bool IsInFrustum = /*UTWeaponFunctionLibrary::IsInFrustumLocation(GetWorld(), HitData.HitResult.Location, 100.0f)*/ true;
	if (IsInFrustum)
	{
		CheckFXOnHit(HitData.HitResult);
	}
}

void ABSProjectile::CheckFXOnHit(const FHitResult& HitResult)
{
	if (GetNetMode() == NM_DedicatedServer) return;

	if (HitResult.bBlockingHit && HitResult.GetActor())
	{
		if (UPhysicalMaterial* PhysMat = HitResult.PhysMaterial.Get())
		{
			// ...
		}
	}
}

void ABSProjectile::EventBulletOut(const FBulletOnOutData& HitData)
{
	if (GetNetMode() == NM_DedicatedServer) return;

	SpawnImpactFXHandler(EImpactType::EIT_Penetrate, HitData.HitResult, HitData.Velocity, HitData.Speed, HitData.ExitLocation);
	OnBulletOut.ExecuteIfBound(this, HitData.HitResult);

	const bool IsInFrustum = /*UTWeaponFunctionLibrary::IsInFrustumLocation(GetWorld(), HitData.HitResult.Location, 100.0f)*/ true;
	if (IsInFrustum)
	{
		CheckFXOnHit(HitData.HitResult);
	}
}

void ABSProjectile::EventBounce(const FBulletOnHitData& HitData)
{
	if (GetNetMode() == NM_DedicatedServer) return;

	SpawnImpactFXHandler(EImpactType::EIT_Ricochet, HitData.HitResult, HitData.Velocity, HitData.Speed, FVector::ZeroVector);
	OnBulletBounce.ExecuteIfBound(this, HitData.HitResult);

	const bool IsInFrustum = /*UTWeaponFunctionLibrary::IsInFrustumLocation(GetWorld(), HitData.HitResult.Location, 100.0f)*/ true;
	if (IsInFrustum)
	{
		CheckFXOnHit(HitData.HitResult);
	}
}

void ABSProjectile::SpawnImpactFXHandler(EImpactType ImpactType, const FHitResult& HitResult, const FVector& InVelocity, float InSpeed, const FVector& ExitLocation)
{
	// ...
}

void ABSProjectile::OnAddedToPool()
{
	if (GetLifeSpan() > 0.0f)
	{
		SetLifeSpan(0.0f);
	}

	SetOwner(nullptr);
	SetInstigator(nullptr);

	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	SetActorHiddenInGame(true);

	SetActorTickEnabled(false);

	ForEachComponent<UActorComponent>(true, [this](UActorComponent* InChild)
	{
		InChild->Deactivate();
	});

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.UnRegisterTickFunction();

	ResetForPool();
	Reset();

	K2_OnAddedToPool();
}

void ABSProjectile::ResetForPool()
{
	if (bInitialIsSet)
	{
		ABSProjectile* MutableDefaultClass = GetMutableDefault<ABSProjectile>(GetClass());
		if (MutableDefaultClass)
		{
			MutableDefaultClass->Diameter = InitialDiameter;
			MutableDefaultClass->Mass = InitialMass;
			MutableDefaultClass->MuzzleVelocity = InitialMuzzleVelocity;
			MutableDefaultClass->Shape = InitialShape;
			MutableDefaultClass->MaxBounceTimes = InitialMaxBounceTimes;
			MutableDefaultClass->LifeTimeDuration = InitialLifeTimeDuration;
			MutableDefaultClass->BaseDamage = InitialBaseDamage;
			MutableDefaultClass->TracerSize = InitialTracerSize;
		}
	}
	else
	{
		const ABSProjectile* DefaultClass = GetDefault<ABSProjectile>(GetClass());
		if (DefaultClass)
		{
			Diameter = DefaultClass->Diameter;
			Mass = DefaultClass->Mass;
			MuzzleVelocity = DefaultClass->MuzzleVelocity;
			Shape = DefaultClass->Shape;
			MaxBounceTimes = DefaultClass->MaxBounceTimes;
			LifeTimeDuration = DefaultClass->LifeTimeDuration;
			BaseDamage = DefaultClass->BaseDamage;
			if (DefaultClass->Tracer)
			{
				TracerSize = DefaultClass->Tracer->GetRelativeScale3D();
			}
		}
	}

	K2_ResetForPool();
}

void ABSProjectile::OnRemovedFromPool()
{
	Reset();

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.RegisterTickFunction(GetWorld()->GetCurrentLevel());

	if (!HasActorBegunPlay())
	{
		DispatchBeginPlay();
	}
	else
	{
		InitializeProjectile();
	}
	SetActorHiddenInGame(false);

	ForEachComponent<UActorComponent>(true, [this](UActorComponent* InChild)
	{
		InChild->Activate();
	});

	K2_OnRemovedFromPool();
}

bool ABSProjectile::IsLocallyControlled() const
{
	return GetInstigator() ? GetInstigator()->IsLocallyControlled() : false;
}

bool ABSProjectile::IsPlayerControlled() const
{
	return GetInstigator() ? GetInstigator()->IsPlayerControlled() : false;
}

void ABSProjectile::UnlistedFromDebugVisual()
{
	bUnlistedFromDebugVisual = true;
}
