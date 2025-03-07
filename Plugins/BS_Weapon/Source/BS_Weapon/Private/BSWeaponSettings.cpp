#include "BSWeaponSettings.h"

UBSWeaponSettings::UBSWeaponSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FDirectoryPath ProjectilesDir;
	ProjectilesDir.Path = TEXT("/BS_Weapon/Projectiles");
	ProjectileSearchPaths.Add(ProjectilesDir);
}

void UBSWeaponSettings::PostInitProperties()
{
	Super::PostInitProperties();
}

#if WITH_EDITOR
void UBSWeaponSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (Name == FName(TEXT("ProjectileSearchPaths")))
	{
		FixSettingPaths();
	}
}
#endif

void UBSWeaponSettings::FixSettingPaths()
{
	for (FDirectoryPath& PathToFix : ProjectileSearchPaths)
	{
		if (!PathToFix.Path.IsEmpty() && !PathToFix.Path.StartsWith(TEXT("/"), ESearchCase::CaseSensitive))
		{
			PathToFix.Path = FString::Printf(TEXT("/Game/%s"), *PathToFix.Path);
		}
	}
}
