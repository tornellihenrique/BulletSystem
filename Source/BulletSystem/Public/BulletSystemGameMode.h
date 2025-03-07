// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BulletSystemGameMode.generated.h"

UCLASS(minimalapi)
class ABulletSystemGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABulletSystemGameMode();

	//~AGameModeBase
	virtual void StartPlay() override;
	//~AGameModeBase
};



