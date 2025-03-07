// Copyright HTech. All Rights Reserved.


#include "THotbarComponent.h"

#include "THotbarCharacterInterface.h"
#include "THotbarItemInterface.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UTHotbarComponent::UTHotbarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Filters.Add(AActor::StaticClass());
	Filters.Add(AActor::StaticClass());
	Filters.Add(AActor::StaticClass());
	Filters.Add(AActor::StaticClass());
	Filters.Add(AActor::StaticClass());
	Filters.Add(AActor::StaticClass());
	Filters.Add(AActor::StaticClass());
	Filters.Add(AActor::StaticClass());
	Filters.Add(AActor::StaticClass());
	
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;

	bWantsInitializeComponent = true;

	SetIsReplicatedByDefault(true);
}

void UTHotbarComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams DefaultParameters;
	DefaultParameters.bIsPushBased = true;
	DefaultParameters.Condition = COND_None;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, Items,			DefaultParameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, SelectedIndex,	DefaultParameters);
}

void UTHotbarComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = CastChecked<ACharacter>(GetOwner());

	Items.SetNum(Filters.Num());
}

void UTHotbarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_UnEquip);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_Equip);
	
	Super::EndPlay(EndPlayReason);
}

void UTHotbarComponent::InitializeComponent()
{
	OwningCharacter = CastChecked<ACharacter>(GetOwner());
}

void UTHotbarComponent::TryAddItem(AActor* Item, int32 InIndex)
{
	if (GetOwnerRole() >= ROLE_Authority)
	{
		const bool bAdded = TryAddItem_Internal(Item, InIndex);
	}
	else
	{
		ServerTryAddItem(Item, InIndex);
	}
}

void UTHotbarComponent::TryRemoveItem(AActor* Item, int32 InIndex)
{
	if (GetOwnerRole() >= ROLE_Authority)
	{
		const bool bRemoved = TryRemoveItem_Internal(Item, InIndex);
	}
	else
	{
		ServerTryRemoveItem(Item, InIndex);
	}
}

void UTHotbarComponent::TryRemoveItemByIndex(int32 InIndex)
{
	if (GetOwnerRole() >= ROLE_Authority)
	{
		const bool bRemoved = TryRemoveItemByIndex_Internal(InIndex);
	}
	else
	{
		ServerTryRemoveItemByIndex(InIndex);
	}
}

void UTHotbarComponent::TrySwapItem(int32 OldIndex, int32 NewIndex)
{
	if (GetOwnerRole() >= ROLE_Authority)
	{
		const bool bSwapped = TrySwapItem_Internal(OldIndex, NewIndex);
	}
	else
	{
		ServerTrySwapItem(OldIndex, NewIndex);
	}
}

bool UTHotbarComponent::HasItem(const AActor* InItem) const
{
	if (!InItem) return false;
	
	for (const auto& Item : Items)
	{
		if (Item == InItem)
		{
			return true;
		}
	}

	return false;
}

int32 UTHotbarComponent::GetItemIndex(const AActor* InItem) const
{
	if (!InItem) return INDEX_NONE;
	
	for (int i = 0; i < Items.Num(); i++)
	{
		if (Items[i] == InItem)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

AActor* UTHotbarComponent::GetSelectedItem() const
{
	if (Items.IsValidIndex(SelectedIndex))
	{
		return Items[SelectedIndex];
	}

	return nullptr;
}

AActor* UTHotbarComponent::GetLastItem() const
{
	if (Items.IsValidIndex(LastIndex))
	{
		return Items[LastIndex];
	}

	return nullptr;
}

AActor* UTHotbarComponent::GetItem(int32 InIndex) const
{
	if (InIndex == INDEX_NONE) return nullptr;
	if (!Items.IsValidIndex(InIndex)) return nullptr;

	return Items[InIndex];
}

bool UTHotbarComponent::HasValidSelectedIndex() const
{
	if (SelectedIndex == INDEX_NONE) return false;
	if (!Items.IsValidIndex(SelectedIndex)) return false;
	return true;
}

bool UTHotbarComponent::IsUnEquipping() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(TimerHandle_UnEquip);
}

void UTHotbarComponent::SetSelectedIndex(int32 NewSelectedIndex)
{
	if (IsUnEquipping()) return;
	
	if (Items.IsValidIndex(NewSelectedIndex))
	{
		if (!Items[NewSelectedIndex])
		{
			NewSelectedIndex = INDEX_NONE;
		}
	}
	else
	{
		NewSelectedIndex = INDEX_NONE;
	}

	if (SelectedIndex == NewSelectedIndex) return;

	ServerSetSelectedIndex(NewSelectedIndex);
}

void UTHotbarComponent::NotifyUnEquip()
{
	if (!OwningCharacter.Get()) return;
	if (!OwningCharacter.IsValid()) return;
	
	ITHotbarCharacterInterface* OwnerCharacterInterface = GetOwner<ITHotbarCharacterInterface>();
	if (!OwnerCharacterInterface) return;
	
	if (SelectedIndex != INDEX_NONE && Items.IsValidIndex(SelectedIndex))
	{
		// if (IsLocallyControlled())
		{
			OwnerCharacterInterface->OnItemChange(GetSelectedItem(), GetLastItem());
		}
	}
	else
	{
		// if (IsLocallyControlled())
		{
			OwnerCharacterInterface->OnItemChange(GetSelectedItem(), GetLastItem());
			OwnerCharacterInterface->OnItemEndedChange(GetSelectedItem(), GetLastItem());
		}
	}
}

void UTHotbarComponent::NotifyEquip()
{
	if (!OwningCharacter.Get()) return;
	if (!OwningCharacter.IsValid()) return;
	
	ITHotbarCharacterInterface* OwnerCharacterInterface = GetOwner<ITHotbarCharacterInterface>();
	if (!OwnerCharacterInterface) return;

	if (bCurveUnEquipEnabled && CurveUnEquipMontage)
	{
		OwningCharacter->StopAnimMontage(CurveUnEquipMontage);
	}
	
	if (GetLastItem())
	{
		if (ITHotbarItemInterface* LastItemInterface = GetLastItem<ITHotbarItemInterface>())
		{
// 			LastItemInterface->StopUnEquipAnimation(true);
// 			LastItemInterface->StopUnEquipAnimation(false);
		}
	}
	
	if (SelectedIndex != INDEX_NONE && Items.IsValidIndex(SelectedIndex))
	{
		if (ITHotbarItemInterface* SelectedItemInterface = GetSelectedItem<ITHotbarItemInterface>())
		{
			const float EquipDuration = FMath::Min(
				SelectedItemInterface->PlayEquipAnimation(OwningCharacter.Get(), true, EquipSpeed),
				SelectedItemInterface->PlayEquipAnimation(OwningCharacter.Get(), false, EquipSpeed));
			
			GetWorld()->GetTimerManager().SetTimer(TimerHandle_Equip, FTimerDelegate::CreateLambda([this]()
			{
				if (!IsValidLowLevel()) return;

				if (ITHotbarCharacterInterface* OwnerCharacterInterface = GetOwner<ITHotbarCharacterInterface>())
				{
					OwnerCharacterInterface->OnItemEndedChange(GetSelectedItem(), GetLastItem());
				}
			}), EquipDuration, false);
		}
	}
	else
	{
		if (GetLastItem())
		{
			if (ITHotbarItemInterface* LastItemInterface = GetLastItem<ITHotbarItemInterface>())
			{
				LastItemInterface->StopEquipAnimation(OwningCharacter.Get(),true);
				LastItemInterface->StopEquipAnimation(OwningCharacter.Get(),false);
			}
		}
	}
}

FName UTHotbarComponent::GetSocket(const bool bSelectedItem) const
{
	if (bSelectedItem && Sockets.IsValidIndex(SelectedIndex))
	{
		return Sockets[SelectedIndex];
	}

	if (!bSelectedItem && Sockets.IsValidIndex(LastIndex))
	{
		return Sockets[LastIndex];
	}

	return NAME_None;
}

bool UTHotbarComponent::TryAddItem_Internal(AActor* Item, int32 InIndex)
{
	if (!Item) return false;
	if (InIndex != -1 && !Items.IsValidIndex(InIndex)) return false;
	if (InIndex != -1 && !Filters.IsValidIndex(InIndex)) return false;

	if (InIndex == -1)
	{
		for (int i = 0; i < Items.Num(); i++)
		{
			if (Items[i]) continue;

			if (Item->IsA(Filters[i]))
			{
				AddItem(Item, i);

				return true;
			}
		}

		if (SelectedIndex == -1) return false;

		InIndex = SelectedIndex;
	}

	if (Item->IsA(Filters[InIndex]))
	{
		AddItem(Item, InIndex);

		return true;
	}
	
	return false;
}

bool UTHotbarComponent::TryRemoveItem_Internal(AActor* Item, int32 InIndex)
{
	if (!Item) return false;

	if (!Items[InIndex]) return false;

	if (Items[InIndex] == Item)
	{
		RemoveItem(InIndex);

		return true;
	}
	
	return false;
}

bool UTHotbarComponent::TryRemoveItemByIndex_Internal(int32 InIndex)
{
	if (!Items[InIndex]) return false;

	RemoveItem(InIndex);

	return true;
}

bool UTHotbarComponent::TrySwapItem_Internal(int32 OldIndex, int32 NewIndex)
{
	if (OldIndex == INDEX_NONE || NewIndex == INDEX_NONE) return false;
	if (!Items.IsValidIndex(OldIndex) || !Items.IsValidIndex(NewIndex)) return false;
	
	AActor* CurrentItem = Items[OldIndex];
	AActor* NewItem = Items[NewIndex];

	if (!CurrentItem || !NewItem) return false;
	if (CurrentItem == NewItem) return false;

	int32 ItemIndex = GetItemIndex(CurrentItem);

	if (ItemIndex == INDEX_NONE) return false;

	if (TryRemoveItem_Internal(CurrentItem, ItemIndex))
	{
		return TryAddItem_Internal(NewItem, ItemIndex);
	}

	return false;
}

AActor* UTHotbarComponent::AddItem(AActor* Item, int32 InIndex)
{
	TArray<AActor*> OldItems = Items;
	
	Items[InIndex] = Item;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Items, this);
	OnRep_Items(OldItems);

	LastAddedItem = Item;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastAddedItem, this);
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_LastAddedItem();
	}

	return Item;
}

void UTHotbarComponent::RemoveItem(int32 InIndex)
{
	TArray<AActor*> OldItems = Items;

	AActor* RemovedItem = Items[InIndex];
	
	Items[InIndex] = nullptr;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, Items, this);
	OnRep_Items(OldItems);

	LastRemovedItem = RemovedItem;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LastRemovedItem, this);
	if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_LastRemovedItem();
	}
}

void UTHotbarComponent::HandleVisual()
{
	// if (!GetCharacterOwner()) return;
	if (Items.Num() <= 0) return;

	for (int i = 0; i < Items.Num(); ++i)
	{
		if (!Items[i]) continue;
		if (i == SelectedIndex) continue;

		if (ITHotbarItemInterface* ItemInterface = Cast<ITHotbarItemInterface>(Items[i]))
		{
			// ItemInterface->BeginInactiveState(!Sockets.IsValidIndex(i) || (Sockets.IsValidIndex(i) && Sockets[i].IsNone())); // @todo: Finish
		}
		
		// Items[i]->AttachToComponent(GetCharacterOwner()->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale,  Sockets[i]);
	}
}

void UTHotbarComponent::ServerTryAddItem_Implementation(AActor* Item, int32 InIndex)
{
	TryAddItem(Item, InIndex);
}

void UTHotbarComponent::ServerTryRemoveItem_Implementation(AActor* Item, int32 InIndex)
{
	TryRemoveItem(Item, InIndex);
}

void UTHotbarComponent::ServerTryRemoveItemByIndex_Implementation(int32 InIndex)
{
	TryRemoveItemByIndex(InIndex);
}

void UTHotbarComponent::ServerTrySwapItem_Implementation(int32 OldIndex, int32 NewIndex)
{
	TrySwapItem(OldIndex, NewIndex);
}

void UTHotbarComponent::ServerSetSelectedIndex_Implementation(const int32 NewSelectedIndex)
{
	LastIndex = SelectedIndex;
	SelectedIndex = NewSelectedIndex;
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, SelectedIndex, this);

	// @todo: Check
	// if (GetNetMode() != NM_DedicatedServer)
	{
		OnRep_SelectedIndex(LastIndex);
	}
}

void UTHotbarComponent::OnRep_Items(TArray<AActor*> OldItems)
{
	HandleVisual();
	
	if (IsLocallyControlled())
	{
		if (SelectedIndex != INDEX_NONE)
		{
			if (Items.IsValidIndex(SelectedIndex) && OldItems.IsValidIndex(SelectedIndex))
			{
				if (OldItems[SelectedIndex] != Items[SelectedIndex])
				{
					// ServerSetSelectedIndex(INDEX_NONE);
				}
			}
		}
	}

	OnItemsUpdated.ExecuteIfBound();
}

void UTHotbarComponent::OnRep_LastAddedItem()
{
	ITHotbarCharacterInterface* OwnerCharacterInterface = GetOwner<ITHotbarCharacterInterface>();
	if (!OwnerCharacterInterface) return;

	if (::IsValid(LastAddedItem))
	{
		OwnerCharacterInterface->OnItemAdded(LastAddedItem);
	}
}

void UTHotbarComponent::OnRep_LastRemovedItem()
{
	ITHotbarCharacterInterface* OwnerCharacterInterface = GetOwner<ITHotbarCharacterInterface>();
	if (!OwnerCharacterInterface) return;

	if (::IsValid(LastRemovedItem))
	{
		OwnerCharacterInterface->OnItemRemoved(LastRemovedItem);
	}
}

void UTHotbarComponent::OnRep_SelectedIndex(int32 OldSelectedIndex)
{
	if (!OwningCharacter.Get()) return;
	if (!OwningCharacter.IsValid()) return;
	
	ITHotbarCharacterInterface* OwnerCharacterInterface = GetOwner<ITHotbarCharacterInterface>();
	if (!OwnerCharacterInterface) return;
	
	LastIndex = OldSelectedIndex;
	
	if (OldSelectedIndex != INDEX_NONE && Items.IsValidIndex(OldSelectedIndex))
	{
		if (ITHotbarItemInterface* LastItemInterface = GetLastItem<ITHotbarItemInterface>())
		{
			// if (IsLocallyControlled())
			{
				OwnerCharacterInterface->OnItemStartedChange(GetSelectedItem(), GetLastItem());
			}

			const bool bEquipping = LastItemInterface->IsPlayingEquipAnimation(OwningCharacter.Get());

			if (OwnerCharacterInterface->ShouldPlayUnEquipAnimation())
			{
				float EquipDelay = LastItemInterface->PlayUnEquipAnimation(OwningCharacter.Get(), false, UnEquipSpeed);
				
				// Play a general curve motion on top of the current animation when draw event is interrupted
				if (IsLocallyControlled() || IsLocallyViewed())
				{
					if (bEquipping && bCurveUnEquipEnabled && CurveUnEquipMontage)
					{
						OwningCharacter->PlayAnimMontage(CurveUnEquipMontage, UnEquipSpeed, NAME_None);
					}
					else
					{
						EquipDelay = LastItemInterface->PlayUnEquipAnimation(OwningCharacter.Get(), true, UnEquipSpeed);
					}
				}

				// Scale the delay time.
				EquipDelay /= UnEquipSpeed;

				if (IsLocallyControlled())
				{
					if (EquipDelay > 0.0f)
					{
						GetWorld()->GetTimerManager().SetTimer(TimerHandle_UnEquip, this, &ThisClass::NotifyUnEquip, EquipDelay * EquipTimerScale, false);
					}
					else
					{
						NotifyUnEquip();
					}
				}
			}
			else
			{
				if (IsLocallyControlled())
				{
					NotifyUnEquip();
				}
			}
		}
		else
		{
			if (IsLocallyControlled())
			{
				NotifyUnEquip();
			}
		}
	}
	else
	{
		if (IsLocallyControlled())
		{
			NotifyUnEquip();
		}
	}
}

ACharacter* UTHotbarComponent::GetCharacterOwner() const
{
	return GetOwner<ACharacter>();
}

bool UTHotbarComponent::IsLocallyControlled() const
{
	return GetCharacterOwner() ? GetCharacterOwner()->IsLocallyControlled() : false;
}

bool UTHotbarComponent::IsLocallyViewed() const
{
	return GetCharacterOwner() ? GetCharacterOwner()->IsLocallyViewed() : false;
}

bool UTHotbarComponent::IsPlayerControlled() const
{
	return GetCharacterOwner() ? GetCharacterOwner()->IsPlayerControlled() : false;
}
