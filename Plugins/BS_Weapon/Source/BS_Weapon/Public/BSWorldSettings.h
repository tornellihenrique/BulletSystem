#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "BSSharedTypes.h"
#include "BSWorldSettings.generated.h"

UCLASS(ClassGroup = "BS", meta = (DisplayName = "BS World Settings"))
class BS_WEAPON_API ABSWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	ABSWorldSettings(const FObjectInitializer& ObjectInitializer);

	//~AWorldSettings
	virtual void NotifyBeginPlay() override;
	virtual void NotifyMatchStarted() override;
	//~AWorldSettings

protected:
	TMap<FName, TObjectPtr<AActor>> PoolActors;
	TMap<FName, TArray<TObjectPtr<AActor>>> ArrayPoolActors;

public:
	void AddPoolActor(const FName& PoolName, AActor* PoolActor);
	void RemovePoolActor(const FName& PoolName);
	AActor* GetPoolActor(const FName& PoolName);

	template<class T>
	T* GetPoolActor(const FName& PoolName);

	void AddArrayPoolActor(const FName& PoolName, AActor* PoolActor);
	void RemoveArrayPoolActor(const FName& PoolName);
	AActor* GetArrayPoolActor(const FName& PoolName);
	bool ArrayPoolExists(const FName& PoolName);

	template<class T>
	T* GetArrayPoolActor(const FName& PoolName);
};

template<class T>
T* ABSWorldSettings::GetPoolActor(const FName& PoolName)
{
	return Cast<T>(GetPoolActor(PoolName));
}

template<class T>
T* ABSWorldSettings::GetArrayPoolActor(const FName& PoolName)
{
	return Cast<T>(GetArrayPoolActor(PoolName));
}