// Copyright HTech. All Rights Reserved.


#include "THotbarItemInterface.h"

float ITHotbarItemInterface::PlayEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/, float InPlayRate /*= 1.0f*/)
{
	return 0.0f;
}

void ITHotbarItemInterface::StopEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/)
{
}

bool ITHotbarItemInterface::IsPlayingEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/)
{
	return false;
}

float ITHotbarItemInterface::PlayUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/, float InPlayRate /*= 1.0f*/)
{
	return 0.0f;
}

void ITHotbarItemInterface::StopUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/)
{
}

bool ITHotbarItemInterface::IsPlayingUnEquipAnimation(APawn* OwningPawn, bool bFirstPerson /*= true*/)
{
	return false;
}
