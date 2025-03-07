// Copyright Epic Games, Inc. All Rights Reserved.

#include "BulletSystemGameMode.h"
#include "BulletSystemCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "BSWorldSettings.h"
#include "Engine/GameInstance.h"
#include "BSWeaponSubsystem.h"
#include "BSProjectile.h"

ABulletSystemGameMode::ABulletSystemGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

void ABulletSystemGameMode::StartPlay()
{
	Super::StartPlay();

	if (UGameInstance* const GI = GetGameInstance())
	{
		if (UBSWeaponSubsystem* WeaponSubsystem = GI->GetSubsystem<UBSWeaponSubsystem>())
		{
			if (ABSWorldSettings* WS = Cast<ABSWorldSettings>(GetWorld()->GetWorldSettings()))
			{
				if (GetNetMode() == NM_DedicatedServer)
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
					SpawnParams.Instigator = nullptr;
					SpawnParams.Owner = nullptr;
					SpawnParams.bNoFail = true;

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
}
