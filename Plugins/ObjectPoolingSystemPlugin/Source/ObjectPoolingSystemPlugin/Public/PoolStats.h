#pragma once

struct FPoolStats
{
	UClass* TypeClass;
	int32 NumberOfPooledObjects;
	int32 TotalPoolCapacity;
	uint64 TotalPoolAllocatedSize;
};
