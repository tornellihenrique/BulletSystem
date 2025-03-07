// Copyright HTech. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "THotbarCharacterInterface.generated.h"

UINTERFACE(MinimalAPI)
class UTHotbarCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class T_HOTBAR_API ITHotbarCharacterInterface
{
	GENERATED_BODY()
	
public:

	virtual void OnItemStartedChange(AActor* Item, AActor* OldItem);
	virtual void OnItemChange(AActor* Item, AActor* OldItem);
	virtual void OnItemEndedChange(AActor* Item, AActor* OldItem);

	virtual void OnItemAdded(AActor* Item);
	virtual void OnItemRemoved(AActor* Item);
	
	virtual bool ShouldPlayEquipAnimation() const;
	virtual bool ShouldPlayUnEquipAnimation() const;
};
