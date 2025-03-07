// Copyright Epic Games, Inc. All Rights Reserved.


#include "BulletSystemPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "BSWorldSettings.h"
#include "Engine/GameInstance.h"
#include "BSWeaponSubsystem.h"
#include "BSProjectile.h"

void ABulletSystemPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() == NM_DedicatedServer) return;

	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		UE_LOG(LogTemp, Warning, TEXT("BeginPlay"));
	}

	if (ABSWorldSettings* WS = Cast<ABSWorldSettings>(GetWorld()->GetWorldSettings()))
	{
		if (UGameInstance* const GI = GetGameInstance())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			SpawnParams.Instigator = nullptr;
			SpawnParams.Owner = nullptr;
			SpawnParams.bNoFail = true;

			if (UBSWeaponSubsystem* WeaponSubsystem = GI->GetSubsystem<UBSWeaponSubsystem>())
			{
				TArray<TSubclassOf<ABSProjectile>> Projectiles;
				WeaponSubsystem->GetProjectiles(Projectiles);

				for (const TSubclassOf<ABSProjectile>& ProjectileClass : Projectiles)
				{
					if (ProjectileClass)
					{
						const FString& ProjectileBlueprintClassName = FPaths::GetBaseFilename(ProjectileClass->GetPathName());

						static const int ProjectileSpawnAmount = 100;

						for (int i = 0; i < ProjectileSpawnAmount; ++i)
						{
							if (ABSProjectile* Projectile = GetWorld()->SpawnActor<ABSProjectile>(ProjectileClass, FTransform::Identity, SpawnParams))
							{
								Projectile->OnAddedToPool();

								WS->AddArrayPoolActor(*ProjectileBlueprintClassName, Projectile);
							}
						}
					}
				}
			}
		}
	}
}