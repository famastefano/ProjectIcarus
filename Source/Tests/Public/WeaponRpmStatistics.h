// Stefano Famà (famastefano@gmail.com)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponRpmStatistics.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCompletion);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCsvDumpFailed);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCsvDumpSucceeded, FString const&, CsvPath);

USTRUCT(BlueprintType)
struct FRpmSnapshot
{
	GENERATED_BODY()

	double Timestamp;
	int Hits;
};

UCLASS()
class TESTS_API AWeaponRpmStatistics : public AActor
{
	GENERATED_BODY()

	enum class EStatus
	{
		Ready,
		Sampling,
		Terminated
	} Status = EStatus::Ready;

	double InitialTimestamp = 0;
	double LastHitTimestamp = 0;
	double TerminationTimestamp = 0;

	void DumpToCsv(FString const& Path) const;

public:
	UPROPERTY(BlueprintAssignable)
	FOnCompletion OnCompletion;

	UPROPERTY(BlueprintAssignable)
	FOnCsvDumpFailed OnCsvDumpFailed;

	UPROPERTY(BlueprintAssignable)
	FOnCsvDumpSucceeded OnCsvDumpSucceeded;

	UPROPERTY(EditAnywhere)
	FString CsvName;

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<UDamageType> ExpectedDamageTypeClass;

	UPROPERTY(EditAnywhere, meta=(UIMin=15, ClampMin=15, UIMax=300, ClampMax=300, Units="s"))
	int SecondsToSample = 15;

	UPROPERTY(EditAnywhere)
	bool TakeSnapshots = false;

	UPROPERTY(EditAnywhere,
		meta=(UIMin=1, ClampMin=1, UIMax=300, ClampMax=300, Units="s", EditCondition="TakeSnapshots"))
	int SecondsBetweenSnapshots = 15;

	UPROPERTY(VisibleInstanceOnly)
	int TotalHits = 0;

	UPROPERTY(VisibleInstanceOnly)
	TArray<FRpmSnapshot> Snapshots;

	void Restart();

	AWeaponRpmStatistics();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator,
	                         AActor* DamageCauser) override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
};
