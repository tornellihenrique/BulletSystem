#include "BSWeaponSubsystem.h"
#include "BSWeaponSettings.h"
#include "BSProjectile.h"

UBSWeaponSubsystem::UBSWeaponSubsystem()
{
	ProjectileLibrary = UObjectLibrary::CreateLibrary(ABSProjectile::StaticClass(), true, GIsEditor);
	ProjectileLibrary->AddToRoot();
}

void UBSWeaponSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ScanAssets();
}

void UBSWeaponSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

TArray<FName> UBSWeaponSubsystem::GetGeneratedBlueprintName(UObjectLibrary* Library) const
{
	TArray<UBlueprintGeneratedClass*> ClassesArray;
	Library->GetObjects<UBlueprintGeneratedClass>(ClassesArray);

	TArray<FName> Output;
	for (int32 i = 0; i < ClassesArray.Num(); ++i)
	{
		UClass* BlueprintClass = ClassesArray[i];
		if (BlueprintClass->GetName().StartsWith("DEV_") || BlueprintClass->GetName().StartsWith("SKEL_")) continue;

		FString Name = BlueprintClass->GetName();
		Name = Name.LeftChop(2);
		Output.Add(*Name);
	}

	return Output;
}

TArray<FName> UBSWeaponSubsystem::GetGeneratedBlueprintNameFiltered(UObjectLibrary* Library, const TArray<FSoftClassPath>& Filters) const
{
	TArray<UBlueprintGeneratedClass*> ClassesArray;
	Library->GetObjects<UBlueprintGeneratedClass>(ClassesArray);

	ClassesArray = ClassesArray.FilterByPredicate([&Filters](UClass* InGeneratedClass)
	{
		const FSoftClassPath& ClassPath = FSoftClassPath(InGeneratedClass->GetPathName());
		return !Filters.Contains(ClassPath);
	});

	TArray<FName> Output;
	for (int32 i = 0; i < ClassesArray.Num(); ++i)
	{
		UClass* BlueprintClass = ClassesArray[i];
		if (BlueprintClass->GetName().StartsWith("DEV_") || BlueprintClass->GetName().StartsWith("SKEL_")) continue;

		FString Name = BlueprintClass->GetName();
		Name = Name.LeftChop(2);
		Output.Add(*Name);
	}

	return Output;
}

FName UBSWeaponSubsystem::GetGeneratedFilename(const FName& ClassName) const
{
	FString Name = ClassName.ToString();
	Name.Append(TEXT("_C")); // Append the "_C" suffix

	return *Name;
}

void UBSWeaponSubsystem::ScanAssets()
{
	static TArray<FString> ProjectilePaths;
	const TArray<FDirectoryPath>& ProjectileSearchPaths = UBSWeaponSettings::Get()->ProjectileSearchPaths;
	for (const FDirectoryPath& Dir : ProjectileSearchPaths)
	{
		ProjectilePaths.Add(Dir.Path);
	}
	ProjectileLibrary->LoadBlueprintsFromPaths(ProjectilePaths);
}

static bool CompareProjectileByName(const TSubclassOf<ABSProjectile>& A, const TSubclassOf<ABSProjectile>& B)
{
	const FSoftClassPath& AClassPath = FSoftClassPath(A.Get()->GetPathName());
	const FSoftClassPath& BClassPath = FSoftClassPath(B.Get()->GetPathName());

	FString NameA = FPaths::GetBaseFilename(AClassPath.ToString());
	FString NameB = FPaths::GetBaseFilename(BClassPath.ToString());

	if (NameA.Left(1) == NameB.Left(1))
	{
		return NameA < NameB;
	}

	return NameA.Left(1) < NameB.Left(1);
}

void UBSWeaponSubsystem::GetProjectiles(TArray<TSubclassOf<ABSProjectile>>& OutClasses)
{
	GetBlueprintClassesFromLibrary<ABSProjectile>(ProjectileLibrary, OutClasses);

	Algo::Sort(OutClasses, CompareProjectileByName);
}

static bool SortByName(const FName& A, const FName& B)
{
	return A.ToString() < B.ToString();
}

TArray<FName> UBSWeaponSubsystem::GetProjectileNames()
{
	TArray<FName> Results = GetGeneratedBlueprintName(ProjectileLibrary);
	Algo::Sort(Results, SortByName);

	return Results;
}

TSubclassOf<ABSProjectile> UBSWeaponSubsystem::GetProjectile(const FName& ClassName)
{
	if (ClassName == NAME_None) return nullptr;
	const FName Name = GetGeneratedFilename(ClassName);

	return GetBlueprintClassFromLibrary<ABSProjectile>(ProjectileLibrary, Name);
}
