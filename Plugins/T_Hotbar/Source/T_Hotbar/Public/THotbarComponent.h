// Copyright HTech. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "THotbarComponent.generated.h"

DECLARE_DELEGATE(FOnItemsUpdated);

DEFINE_LOG_CATEGORY_STATIC(LogHotbarComponent, Log, All);

UCLASS(ClassGroup=(Hotbar), meta=(BlueprintSpawnableComponent), DisplayName = "Hotbar Component")
class T_HOTBAR_API UTHotbarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs", meta = (ExposeOnSpawn = "true"))
	TArray<TSubclassOf<AActor>> Filters;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs", meta = (ExposeOnSpawn = "true"))
	TArray<FName> Sockets;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Animation", meta = (ExposeOnSpawn = "true"))
	float EquipSpeed{1.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Animation", meta = (ExposeOnSpawn = "true"))
	float UnEquipSpeed{1.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Animation", meta = (ExposeOnSpawn = "true"))
	float EquipTimerScale{1.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Animation", meta = (ExposeOnSpawn = "true"))
	bool bCurveUnEquipEnabled{true};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configs|Animation", meta = (ExposeOnSpawn = "true", EditCondition = "bCurveUnEquipEnabled", EditConditionHides))
	TObjectPtr<UAnimMontage> CurveUnEquipMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", ReplicatedUsing = OnRep_Items)
	TArray<AActor*> Items;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", ReplicatedUsing = OnRep_LastAddedItem)
	AActor* LastAddedItem;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", ReplicatedUsing = OnRep_LastRemovedItem)
	AActor* LastRemovedItem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", ReplicatedUsing = OnRep_SelectedIndex)
	int32 SelectedIndex{INDEX_NONE};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	int32 LastIndex{INDEX_NONE};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	TWeakObjectPtr<ACharacter> OwningCharacter;
	
	UTHotbarComponent(const FObjectInitializer& ObjectInitializer);

public:

	FOnItemsUpdated OnItemsUpdated;

protected:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void InitializeComponent() override;

	//~Usage
public:
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void TryAddItem(AActor* Item, int32 InIndex);
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void TryRemoveItem(AActor* Item, int32 InIndex);
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void TryRemoveItemByIndex(int32 InIndex);
	UFUNCTION(BlueprintCallable, Category = "Hotbar")
	void TrySwapItem(int32 OldIndex, int32 NewIndex);
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	bool HasItem(const AActor* InItem) const;
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	int32 GetItemIndex(const AActor* InItem) const;

	UFUNCTION(BlueprintPure, Category = "Hotbar")
	AActor* GetSelectedItem() const;
	template < class T >
	T* GetSelectedItem() const { return Cast<T>(GetSelectedItem()); }
	
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	AActor* GetLastItem() const;
	template < class T >
	T* GetLastItem() const { return Cast<T>(GetLastItem()); }
	
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	AActor* GetItem(int32 InIndex) const;
	
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	bool HasValidSelectedIndex() const;
	
	UFUNCTION(BlueprintPure, Category = "Hotbar")
	bool IsUnEquipping() const;

	UFUNCTION(BlueprintCallable, Category = "Hotbar", Meta = (AdvancedDisplay = "0"))
	void SetSelectedIndex(int32 NewSelectedIndex = -1);

	void NotifyUnEquip();
	void NotifyEquip();
	
	FName GetSocket(const bool bSelectedItem) const;
	
	/* Server only */
	virtual bool TryAddItem_Internal(AActor* Item, int32 InIndex);
	/* Server only */
	virtual bool TryRemoveItem_Internal(AActor* Item, int32 InIndex);
	/* Server only */
	virtual bool TryRemoveItemByIndex_Internal(int32 InIndex);
	/* Server only */
	virtual bool TrySwapItem_Internal(int32 OldIndex, int32 NewIndex);
	//~Usage

protected:
	AActor* AddItem(AActor* Item, int32 InIndex);
	void RemoveItem(int32 InIndex);

	void HandleVisual();

	FTimerHandle TimerHandle_UnEquip;
	FTimerHandle TimerHandle_Equip;

protected:
	UFUNCTION(Server, Reliable)
	void ServerTryAddItem(AActor* Item, int32 InIndex);
	UFUNCTION(Server, Reliable)
	void ServerTryRemoveItem(AActor* Item, int32 InIndex);
	UFUNCTION(Server, Reliable)
	void ServerTryRemoveItemByIndex(int32 InIndex);
	UFUNCTION(Server, Reliable)
	void ServerTrySwapItem(int32 OldIndex, int32 NewIndex);

	UFUNCTION(Server, Reliable)
	void ServerSetSelectedIndex(const int32 NewSelectedIndex);
	
	UFUNCTION()
	void OnRep_Items(TArray<AActor*> OldItems);
	UFUNCTION()
	void OnRep_LastAddedItem();
	UFUNCTION()
	void OnRep_LastRemovedItem();
	UFUNCTION()
	void OnRep_SelectedIndex(int32 OldSelectedIndex);

protected:
	ACharacter* GetCharacterOwner() const;
	
	bool IsLocallyControlled() const;
	bool IsLocallyViewed() const;
	bool IsPlayerControlled() const;
	
};
