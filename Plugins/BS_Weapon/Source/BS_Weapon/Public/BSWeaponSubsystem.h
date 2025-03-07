#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/ObjectLibrary.h"
#include "Engine/StreamableManager.h"
#include "BSWeaponSubsystem.generated.h"

class ABSProjectile;

UCLASS(ClassGroup = "BS", meta = (DisplayName = "Weapon Subsystem"))
class BS_WEAPON_API UBSWeaponSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	TObjectPtr<UObjectLibrary> ProjectileLibrary;

	FStreamableManager StreamableManager;

public:
	UBSWeaponSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	TArray<FName> GetGeneratedBlueprintName(UObjectLibrary* Library) const;
	TArray<FName> GetGeneratedBlueprintNameFiltered(UObjectLibrary* Library, const TArray<FSoftClassPath>& Filters) const;
	FName GetGeneratedFilename(const FName& ClassName) const;

	template<typename T>
	void GetAssetsFromLibrary(UObjectLibrary* Library, TArray<T*>& OutAssets);

	template<typename T>
	TSubclassOf<T> GetBlueprintClassFromLibrary(UObjectLibrary* Library, const FName& ClassName);

	template<typename T>
	void GetBlueprintClassesFromLibrary(UObjectLibrary* Library, TArray<TSubclassOf<T>>& OutClasses);

protected:
	virtual void ScanAssets();

public:
	UFUNCTION(BlueprintCallable, Category = "Weapon Subsystem")
	void GetProjectiles(TArray<TSubclassOf<ABSProjectile>>& OutClasses);

	UFUNCTION(BlueprintPure, Category = "Weapon Subsystem")
	TArray<FName> GetProjectileNames();

	UFUNCTION(BlueprintPure, Category = "Weapon Subsystem")
	TSubclassOf<ABSProjectile> GetProjectile(const FName& ClassName);

};

template<typename T>
void UBSWeaponSubsystem::GetAssetsFromLibrary(UObjectLibrary* Library, TArray<T*>& OutAssets)
{
	OutAssets.Empty();

	TArray<FAssetData> AssetDataArray;
	Library->GetAssetDataList(AssetDataArray);

	for (int32 i = 0; i < AssetDataArray.Num(); ++i)
	{
		FAssetData& AssetData = AssetDataArray[i];

		T* Asset = Cast<T>(StreamableManager.LoadSynchronous(AssetData.GetObjectPathString()));
		if (Asset)
		{
			OutAssets.Add(Asset);
		}
	}
}

template<typename T>
TSubclassOf<T>
UBSWeaponSubsystem::GetBlueprintClassFromLibrary(UObjectLibrary* Library, const FName& ClassName)
{
	TArray<UBlueprintGeneratedClass*> ClassesArray;
	Library->GetObjects<UBlueprintGeneratedClass>(ClassesArray);

	for (int32 i = 0; i < ClassesArray.Num(); ++i)
	{
		UClass* BlueprintClass = ClassesArray[i];
		if (BlueprintClass->GetName().StartsWith("DEV_") || BlueprintClass->GetName().StartsWith("SKEL_")) continue;

		if (BlueprintClass->GetFName() == ClassName)
		{
			return BlueprintClass;
		}
	}

	return nullptr;
}

template<typename T>
void UBSWeaponSubsystem::GetBlueprintClassesFromLibrary(UObjectLibrary* Library, TArray<TSubclassOf<T>>& OutClasses)
{
	OutClasses.Empty();

	TArray<UBlueprintGeneratedClass*> ClassesArray;
	Library->GetObjects<UBlueprintGeneratedClass>(ClassesArray);

	for (int32 i = 0; i < ClassesArray.Num(); ++i)
	{
		UBlueprintGeneratedClass* BlueprintClass = ClassesArray[i];
		if (BlueprintClass->GetName().StartsWith("DEV_") || BlueprintClass->GetName().StartsWith("SKEL_")) continue;

		OutClasses.Add(BlueprintClass);
	}
}
