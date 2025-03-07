// Copyright HTech. All Rights Reserved.


#include "THotbarCharacterInterface.h"

void ITHotbarCharacterInterface::OnItemStartedChange(AActor* Item, AActor* OldItem)
{
}

void ITHotbarCharacterInterface::OnItemChange(AActor* Item, AActor* OldItem)
{
}

void ITHotbarCharacterInterface::OnItemEndedChange(AActor* Item, AActor* OldItem)
{
}

void ITHotbarCharacterInterface::OnItemAdded(AActor* Item)
{

}

void ITHotbarCharacterInterface::OnItemRemoved(AActor* Item)
{

}

bool ITHotbarCharacterInterface::ShouldPlayEquipAnimation() const
{
	return true;
}

bool ITHotbarCharacterInterface::ShouldPlayUnEquipAnimation() const
{
	return true;
}
