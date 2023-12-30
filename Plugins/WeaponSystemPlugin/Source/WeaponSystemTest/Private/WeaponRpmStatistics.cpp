// Stefano Famà (famastefano@gmail.com)

#include "WeaponRpmStatistics.h"

#include "LogWeaponSystemTest.h"

#include "Engine/DamageEvents.h"

#include "Logging/StructuredLog.h"

void AWeaponRpmStatistics::DumpToCsv(const FString& Path) const
{
	UE_LOGFMT(LogWeaponSystemTest, Display, "CSV path `{Path}`", Path);

	FString Data;
	Data.Append(TEXT("Time (Seconds);Hits;RPM;% Error\n"));
	for (const auto& [Timestamp, Hits, FireRate, Error] : Snapshots)
	{
		Data.Appendf(TEXT("%.1f;%d;%.1f;%.1f%%\n"), Timestamp, Hits, FireRate, Error);
	}

	if (FFileHelper::SaveStringToFile(Data, GetData(Path), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		if (OnCsvDumpSucceeded.IsBound())
		{
			OnCsvDumpSucceeded.Broadcast(Path);
		}
	}
	else
	{
		if (OnCsvDumpFailed.IsBound())
		{
			OnCsvDumpFailed.Broadcast();
		}
	}
}

void AWeaponRpmStatistics::Restart()
{
	Status = EStatus::Ready;
	InitialTimestamp = 0;
	LastHitTimestamp = 0;
	TerminationTimestamp = 0;
	NextSnapshotTimestamp = 0;
	TotalHits = 0;
	Snapshots.Reset();
}

AWeaponRpmStatistics::AWeaponRpmStatistics()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
}

float AWeaponRpmStatistics::TakeDamage(float, const FDamageEvent& DamageEvent,
                                       AController*, AActor*)
{
	if (ExpectedDamageTypeClass.IsValid())
	{
		if (DamageEvent.DamageTypeClass.Get() == ExpectedDamageTypeClass.Get())
		{
			if (Status == EStatus::Terminated)
			{
				return 0;
			}

			if (Status == EStatus::Ready)
			{
				const double Now = GetWorld()->GetTimeSeconds();
				Status = EStatus::Sampling;
				InitialTimestamp = Now;
				TerminationTimestamp = Now + SecondsToSample;
				NextSnapshotTimestamp = Now + SecondsBetweenSnapshots;
				UE_LOGFMT(LogWeaponSystemTest, Display, "{CsvName} started sampling at {Timestamp}.", CsvName, Now);
			}

			++TotalHits;
		}
		else
		{
			UE_LOGFMT(LogWeaponSystemTest, Warning, "Recieved damage from an unexpected DamageType: {DamageType}",
			          DamageEvent.DamageTypeClass->GetDescription());
		}
	}
	else
	{
		UE_LOGFMT(LogWeaponSystemTest, Error, "This class can't take samples without ExpectedDamageTypeClass being set.");
	}

	return 0;
}

void AWeaponRpmStatistics::Tick(float)
{
	if (Status != EStatus::Sampling)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (TakeSnapshots && Now >= NextSnapshotTimestamp)
	{
		NextSnapshotTimestamp += SecondsBetweenSnapshots;
		const double Timestamp = Now - InitialTimestamp;
		Snapshots.Add({
			Timestamp, TotalHits, CalculateFireRate(Timestamp, TotalHits), CalculateError(Timestamp, TotalHits)
		});
	}

	if (Now >= TerminationTimestamp)
	{
		UE_LOGFMT(LogWeaponSystemTest, Display, "{ActorName} Terminated", GetName());
		Status = EStatus::Terminated;
		const double Timestamp = Now - InitialTimestamp;
		Snapshots.Add({
			Timestamp, TotalHits, CalculateFireRate(Timestamp, TotalHits), CalculateError(Timestamp, TotalHits)
		});

		const FString DirTree = FPaths::Combine(FPaths::ProjectDir(),TEXT("Csv"));
		if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*DirTree))
		{
			const FString FileName = "WeaponRpmStatistics_" + CsvName + "_%Y%m%d_%H%M%S.csv";
			const FString CsvPath = FPaths::Combine(DirTree, FDateTime::Now().ToFormattedString(*FileName));
			DumpToCsv(CsvPath);
		}
		else
		{
			if (OnCsvDumpFailed.IsBound())
			{
				OnCsvDumpFailed.Broadcast();
			}
		}

		if (OnCompletion.IsBound())
		{
			OnCompletion.Broadcast();
		}
	}
}

void AWeaponRpmStatistics::BeginPlay()
{
#if WITH_EDITOR
	if (CsvName.IsEmpty())
	{
		CsvName = GetActorLabel();
	}
#endif
	Super::BeginPlay();
}

double AWeaponRpmStatistics::CalculateFireRate(double Timestamp, int Hits) const
{
	return Hits / (Timestamp / 60);
}

double AWeaponRpmStatistics::CalculateError(double Timestamp, int Hits) const
{
	const double ObservedFireRate = CalculateFireRate(Timestamp, Hits);
	const double AbsoluteError = FMath::Abs(ExpectedRpm - ObservedFireRate);
	return (AbsoluteError / ExpectedRpm) * 100;
}
