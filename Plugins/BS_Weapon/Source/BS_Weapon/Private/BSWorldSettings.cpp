#include "BSWorldSettings.h"

ABSWorldSettings::ABSWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ABSWorldSettings::NotifyBeginPlay()
{
	Super::NotifyBeginPlay();
}

void ABSWorldSettings::NotifyMatchStarted()
{
	Super::NotifyMatchStarted();
}

void ABSWorldSettings::AddPoolActor(const FName& PoolName, AActor* PoolActor)
{
	if (!GetPoolActor(PoolName))
	{
		PoolActors.Add(PoolName, PoolActor);
	}
}

void ABSWorldSettings::RemovePoolActor(const FName& PoolName)
{
	if (PoolActors.Contains(PoolName))
	{
		PoolActors.Remove(PoolName);
	}
}

AActor* ABSWorldSettings::GetPoolActor(const FName& PoolName)
{
	if (PoolActors.Contains(PoolName))
	{
		return PoolActors.FindRef(PoolName);
	}

	return nullptr;
}

void ABSWorldSettings::AddArrayPoolActor(const FName& PoolName, AActor* PoolActor)
{
	if (!PoolActor) return;
	if (ArrayPoolActors.Contains(PoolName))
	{
		TArray<TObjectPtr<AActor>>& ArrayPool = ArrayPoolActors.FindChecked(PoolName);
		const int32 FoundIndex = ArrayPool.IndexOfByPredicate([PoolActor](const TObjectPtr<AActor>& InActor)
		{
			if (InActor.Get())
			{
				return InActor.Get() == PoolActor;
			}

			return false;
		});

		if (FoundIndex == INDEX_NONE)
		{
			ArrayPool.Add(PoolActor);
		}
	}
	else
	{
		TArray<TObjectPtr<AActor>>& ArrayPool = ArrayPoolActors.Add(PoolName);
		ArrayPool.Add(PoolActor);
	}
}

void ABSWorldSettings::RemoveArrayPoolActor(const FName& PoolName)
{
	if (ArrayPoolActors.Contains(PoolName))
	{
		ArrayPoolActors.Remove(PoolName);
	}
}

AActor* ABSWorldSettings::GetArrayPoolActor(const FName& PoolName)
{
	AActor* ActorRef = nullptr;

	if (ArrayPoolActors.Contains(PoolName))
	{
		TArray<TObjectPtr<AActor>>& ArrayPool = ArrayPoolActors.FindChecked(PoolName);
		for (int32 i = 0; i < ArrayPool.Num(); i++)
		{
			if (ArrayPool.IsValidIndex(i) && ArrayPool[i] != nullptr)
			{
				ActorRef = ArrayPool[i].Get();
				ArrayPool.RemoveAtSwap(i);

				break;
			}
		}
	}

	return ActorRef;
}

bool ABSWorldSettings::ArrayPoolExists(const FName& PoolName)
{
	return ArrayPoolActors.Contains(PoolName);
}
