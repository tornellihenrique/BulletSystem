// Copyright HTech. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "THotbarItemInterface.generated.h"

UINTERFACE(MinimalAPI)
class UTHotbarItemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class T_HOTBAR_API ITHotbarItemInterface
{
	GENERATED_BODY()
	
public:
	
	virtual float PlayEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true, float InPlayRate = 1.0f);
	virtual void StopEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true);
	virtual bool IsPlayingEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true);
	
	virtual float PlayUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true, float InPlayRate = 1.0f);
	virtual void StopUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true);
	virtual bool IsPlayingUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson = true);
	
};
