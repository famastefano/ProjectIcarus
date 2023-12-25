// Stefano Famà (famastefano@gmail.com)

#include "WeaponRpmStatistics.h"

#include "LogIcarusTests.h"

#include "Engine/DamageEvents.h"

#include "Logging/StructuredLog.h"

void AWeaponRpmStatistics::DumpToCsv(FString const& Path) const
{
	UE_LOGFMT(LogIcarusTests, Display, "CSV path `{Path}`", Path);

	FString Data;
	Data.Append(TEXT("Time (Seconds);Hits;RPM\n"));
	for (const auto& [Timestamp, Hits] : Snapshots)
		Data.Appendf(TEXT("%.3f;%d;%.3f\n"), Timestamp, Hits, Hits / (Timestamp / 60));

	if (FFileHelper::SaveStringToFile(Data, GetData(Path), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		if (OnCsvDumpSucceeded.IsBound())
			OnCsvDumpSucceeded.Broadcast(Path);
	}
	else
	{
		if (OnCsvDumpFailed.IsBound())
			OnCsvDumpFailed.Broadcast();
	}
}

void AWeaponRpmStatistics::Restart()
{
	Status = EStatus::Ready;
	InitialTimestamp = 0;
	LastHitTimestamp = 0;
	TerminationTimestamp = 0;
	TotalHits = 0;
	Snapshots.Reset();
}

AWeaponRpmStatistics::AWeaponRpmStatistics()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

float AWeaponRpmStatistics::TakeDamage(float, const FDamageEvent& DamageEvent,
                                       AController*, AActor*)
{
	if (ExpectedDamageTypeClass.IsValid())
	{
		if (DamageEvent.DamageTypeClass.Get() == ExpectedDamageTypeClass.Get())
		{
			if (Status == EStatus::Terminated)
				return 0;

			if (Status == EStatus::Ready)
			{
				double const Now = GetWorld()->GetTimeSeconds();
				Status = EStatus::Sampling;
				InitialTimestamp = Now;
				TerminationTimestamp = Now + SecondsToSample;
				UE_LOGFMT(LogIcarusTests, Display, "{CsvName} started sampling at {Timestamp}.", CsvName, Now);
			}

			++TotalHits;
		}
		else
		{
			UE_LOGFMT(LogIcarusTests, Warning, "Recieved damage from an unexpected DamageType: {DamageType}",
			          DamageEvent.DamageTypeClass->GetDescription());
		}
	}
	else
	{
		UE_LOGFMT(LogIcarusTests, Error, "This class can't take samples without ExpectedDamageTypeClass being set.");
	}

	return 0;
}

void AWeaponRpmStatistics::Tick(float)
{
	if (Status != EStatus::Sampling)
		return;

	double const Now = GetWorld()->GetTimeSeconds();
	if (TakeSnapshots && Now >= InitialTimestamp + SecondsBetweenSnapshots * (Snapshots.Num() + 1))
		Snapshots.Add({Now - InitialTimestamp, TotalHits});

	if (Now >= TerminationTimestamp)
	{
		UE_LOGFMT(LogIcarusTests, Display, "{ActorName} Terminated", GetName());
		Status = EStatus::Terminated;
		Snapshots.Add({Now - InitialTimestamp, TotalHits});

		FString const DirTree = FPaths::Combine(FPaths::ProjectDir(),TEXT("csv"));
		if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*DirTree))
		{
			FString const FileName = "WeaponRpmStatistics_" + CsvName + "_%Y%m%d_%H%M%S.csv";
			FString const CsvPath = FPaths::Combine(DirTree, FDateTime::Now().ToFormattedString(*FileName));
			DumpToCsv(CsvPath);
		}
		else
		{
			if (OnCsvDumpFailed.IsBound())
				OnCsvDumpFailed.Broadcast();
		}

		if (OnCompletion.IsBound())
			OnCompletion.Broadcast();
	}
}

void AWeaponRpmStatistics::BeginPlay()
{
	if(CsvName.IsEmpty())
		CsvName = GetActorLabel();
	Super::BeginPlay();
}
