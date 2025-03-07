#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BSSharedTypes.h"
#include "BSWeaponSettings.generated.h"

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Weapon Settings"))
class BS_WEAPON_API UBSWeaponSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category = "Configs", meta = (LongPackageName))
	TArray<FDirectoryPath> ProjectileSearchPaths;

	UPROPERTY(config, EditAnywhere, Category = "Configs")
	FPenetrateData PenetrateSurfaceTypes[SurfaceType_Max];

public:
	UBSWeaponSettings(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual FName GetCategoryName() const override { return FName(TEXT("Bullet System")); }

	static UBSWeaponSettings* Get() { return CastChecked<UBSWeaponSettings>(StaticClass()->GetDefaultObject()); }

protected:
	void FixSettingPaths();

};
