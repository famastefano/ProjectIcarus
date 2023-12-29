#pragma once

#include "Stats/Stats.h"
#include "Trace/Trace.h"

UE_TRACE_CHANNEL_EXTERN(WeaponSystemChannel, WEAPONSYSTEM_API);
DECLARE_STATS_GROUP_VERBOSE(TEXT("Weapon System"), STATGROUP_WeaponSystem, STATCAT_WeaponSystem);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Tick"), STAT_WeaponSystem_Tick, STATGROUP_WeaponSystem,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Fire"), STAT_WeaponSystem_Fire, STATGROUP_WeaponSystem,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Notify"), STAT_WeaponSystem_Notify, STATGROUP_WeaponSystem,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Update Magazine"), STAT_WeaponSystem_UpdateMagazine, STATGROUP_WeaponSystem,);
