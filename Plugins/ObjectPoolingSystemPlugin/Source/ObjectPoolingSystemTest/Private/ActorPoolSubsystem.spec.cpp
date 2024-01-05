#include "ActorPoolSubsystem.h"

#include "Logging/StructuredLog.h"

#include "LogObjectPoolingSystemTest.h"
#include "TestWorldActor.h"
#include "PoolTestActor_Alice.h"
#include "PoolTestActor_Bob.h"

#include "TestWorldSubsystem.h"

#include "Algo/AllOf.h"
#include "Algo/Count.h"

#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FActorPoolSubsystem_Spec, "ObjectPoolingSystem.Runtime.ActorPooling",
                  EAutomationTestFlags::ApplicationContextMask
                  | EAutomationTestFlags::MediumPriority
                  | EAutomationTestFlags::ProductFilter)

	TObjectPtr<UTestWorldSubsystem> TestSubsystem;
UWorld* WorldContextObject;
	FTestWorldHelper World;
int32 RandSeed = static_cast<int32>(FDateTime::Now().GetTicks());

template <typename TContainer>
bool AreActorsValid(const TContainer& Container)
{
	return Algo::AllOf(Container, [](AActor* Actor) { return Actor != nullptr; });
}

template <typename TContainer>
bool AreActorsUnique(const TContainer& Container)
{
	return Algo::AllOf(Container, [&Container](AActor* Actor)
	{
		return Algo::Count(Container, Actor) == 1;
	});
}

template <typename TContainer>
bool AreTheSameActor(AActor* Actor, const TContainer& Container)
{
	return Algo::AllOf(Container, [Actor](AActor* OtherActor) { return Actor == OtherActor; });
};

int32 GetRandomValue()
{
	constexpr int MaximumRandomValue = 100;
	return FMath::RandHelper(MaximumRandomValue) + 1;
}

END_DEFINE_SPEC(FActorPoolSubsystem_Spec)

void FActorPoolSubsystem_Spec::Define()
{
	using ClassPair = TPair<UClass*, int>;

	Describe("The Actor Pool", [this]
	{
		BeforeEach([this]
		{
			UE_LOGFMT(LogObjectPoolingSystemTest, Log, "Test {Name} started. RandSeed = {Seed}", GetTestFullName(),
			          RandSeed);
			FMath::RandInit(RandSeed);
			if (!TestSubsystem)
			{
				TestSubsystem = GEngine->GetEngineSubsystem<UTestWorldSubsystem>();
			}
			World = TestSubsystem->GetPrivateWorld("FActorPoolSubsystem_Spec_World");
			WorldContextObject = World->GetWorld();
		});

		It("Should be empty, if it has never been used", [this]
		{
			const auto& Stats = UActorPoolSubsystem::GetAllPoolStats(WorldContextObject);
			TestTrueExpr(Stats.IsEmpty());
		});

		Describe("When populating the pool", [this]
		{
			It("Should have N Actors available, of the same Class", [this]
			{
				UActorPoolSubsystem::PopulatePool(WorldContextObject, ATestWorldActor::StaticClass(), 10);
				auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ATestWorldActor::StaticClass());
				TestTrueExpr(Stats.TypeClass == ATestWorldActor::StaticClass());
				TestTrueExpr(Stats.NumberOfPooledObjects == 10);
			});

			It("Should have N Actors available, of multiple Classes", [this]
			{
				TArray ActorClasses = {
					ClassPair{ATestWorldActor::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Alice::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Bob::StaticClass(), GetRandomValue()},
				};

				for (auto [ActorClass, Count] : ActorClasses)
				{
					UActorPoolSubsystem::PopulatePool(WorldContextObject, ActorClass, Count);
				}

				for (auto [ActorClass, Count] : ActorClasses)
				{
					auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ActorClass);
					TestTrueExpr(Stats.TypeClass == ActorClass);
					TestTrueExpr(Stats.NumberOfPooledObjects == Count);
				}
			});

			It("Should have append new Actors to the pool, if they are of the same class", [this]
			{
				int64 Count = 0;
				for (int Iterations = 0; Iterations < 5; ++Iterations)
				{
					int32 ActorsToSpawn = GetRandomValue();
					UActorPoolSubsystem::PopulatePool(WorldContextObject,
					                                  ATestWorldActor::StaticClass(),
					                                  ActorsToSpawn);
					Count += ActorsToSpawn;
				}

				auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ATestWorldActor::StaticClass());
				TestTrueExpr(Stats.TypeClass == ATestWorldActor::StaticClass());
				TestTrueExpr(Stats.NumberOfPooledObjects == Count);
			});
		});

		Describe("When spawning or acquiring actors from the pool", [this]
		{
			It("Should return new Actors and keep the pool empty", [this]
			{
				UClass* ActorClass = ATestWorldActor::StaticClass();
				TArray<AActor*> SpawnedActors;

				SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
				SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
				SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));

				TestTrueExpr(AreActorsValid(SpawnedActors) && AreActorsUnique(SpawnedActors));

				auto Stats = UActorPoolSubsystem::GetAllPoolStats(WorldContextObject);
				TestTrueExpr(Stats.IsEmpty());
			});

			It("Should return new Actors and keep the pool empty, even with multiple classes", [this]
			{
				TArray ActorClasses{
					ClassPair{ATestWorldActor::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Alice::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Bob::StaticClass(), GetRandomValue()},
				};

				TArray<AActor*> SpawnedActors;
				for (auto [ActorClass, Count] : ActorClasses)
				{
					for (decltype(Count) SpawnCount = 0; SpawnCount < Count; ++SpawnCount)
					{
						SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
					}
				}

				TestTrueExpr(AreActorsValid(SpawnedActors) && AreActorsUnique(SpawnedActors));

				auto Stats = UActorPoolSubsystem::GetAllPoolStats(WorldContextObject);
				TestTrueExpr(Stats.IsEmpty());
			});

			It("Should return pooled Actors, if the pool isnt empty", [this]
			{
				int PoolSize = GetRandomValue() + 1; // Avoids 1
				int SpawnSize = PoolSize - 1;

				UClass* ActorClass = ATestWorldActor::StaticClass();
				UActorPoolSubsystem::PopulatePool(WorldContextObject, ActorClass, PoolSize);

				TArray<AActor*> SpawnedActors;
				for (int SpawnCount = 0; SpawnCount < SpawnSize; ++SpawnCount)
				{
					SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
				}

				TestTrueExpr(AreActorsValid(SpawnedActors) && AreActorsUnique(SpawnedActors));
				auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ActorClass);
				TestTrueExpr(Stats.TypeClass == ActorClass);
				TestTrueExpr(Stats.NumberOfPooledObjects == PoolSize - SpawnSize);
			});

			It("Should return pooled Actors, if the pool isnt empty, from separate pools", [this]
			{
				TArray ActorClasses{
					ClassPair{ATestWorldActor::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Alice::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Bob::StaticClass(), GetRandomValue()},
				};

				for (auto [ActorClass, Count] : ActorClasses)
				{
					UActorPoolSubsystem::PopulatePool(WorldContextObject, ActorClass, Count * 2);
				}

				TArray<AActor*> SpawnedActors;
				for (auto [ActorClass, Count] : ActorClasses)
				{
					for (int SpawnCount = 0; SpawnCount < Count; ++SpawnCount)
					{
						SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
					}
				}

				TestTrueExpr(AreActorsValid(SpawnedActors) && AreActorsUnique(SpawnedActors));

				for (auto [ActorClass, Count] : ActorClasses)
				{
					auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ActorClass);
					TestTrueExpr(Stats.TypeClass == ActorClass);
					TestTrueExpr(Stats.NumberOfPooledObjects == Count);
				}
			});

			It("Should return the same Actor, if spawned and released continuously", [this]
			{
				int32 Iterations = GetRandomValue();
				UClass* ActorClass = ATestWorldActor::StaticClass();
				TArray<AActor*> SpawnedActors;
				SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
				for (int Count = 0; Count < Iterations; ++Count)
				{
					UActorPoolSubsystem::DestroyOrReleaseToPool(WorldContextObject, SpawnedActors.Last());
					SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
				}

				TestTrueExpr(AreTheSameActor(SpawnedActors[0], SpawnedActors));
			});

			It(
				"Should return the same Actor, if spawned and released continuously, but only if they are of the same Class",
				[this]
				{
					TArray ActorClasses{
						ClassPair{ATestWorldActor::StaticClass(), GetRandomValue()},
						ClassPair{APoolTestActor_Alice::StaticClass(), GetRandomValue()},
						ClassPair{APoolTestActor_Bob::StaticClass(), GetRandomValue()},
					};

					for (auto [ActorClass, Count] : ActorClasses)
					{
						UActorPoolSubsystem::PopulatePool(WorldContextObject, ActorClass, Count * 2);
					}

					for (auto [ActorClass, Count] : ActorClasses)
					{
						TArray<AActor*> SpawnedActors;
						SpawnedActors.Add(UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
						for (int Iterations = 0; Iterations < Count; ++Iterations)
						{
							UActorPoolSubsystem::DestroyOrReleaseToPool(WorldContextObject, SpawnedActors.Last());
							SpawnedActors.Add(
								UActorPoolSubsystem::SpawnOrAcquireFromPool(WorldContextObject, ActorClass));
						}
						TestTrueExpr(AreTheSameActor(SpawnedActors[0], SpawnedActors));
					}
				});
		});

		Describe("When releasing actors to the pool", [this]
		{
			It("Should increase the pool size by N, if the pool were empty", [this]
			{
				UClass* ActorClass = ATestWorldActor::StaticClass();
				int32 Count = GetRandomValue();

				auto AllStats = UActorPoolSubsystem::GetAllPoolStats(WorldContextObject);
				TestTrueExpr(AllStats.IsEmpty());

				for (int32 Iterations = 0; Iterations < Count; ++Iterations)
				{
					AActor* Actor = World->SpawnActor<AActor>(ActorClass);
					UActorPoolSubsystem::DestroyOrReleaseToPool(WorldContextObject, Actor);
				}

				auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ActorClass);
				TestTrueExpr(Stats.TypeClass == ActorClass);
				TestTrueExpr(Stats.NumberOfPooledObjects == Count);
			});

			It("Should increase the pool size by N, if the pool were empty, for each pool", [this]
			{
				TArray ActorClasses{
					ClassPair{ATestWorldActor::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Alice::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Bob::StaticClass(), GetRandomValue()},
				};

				auto AllStats = UActorPoolSubsystem::GetAllPoolStats(WorldContextObject);
				TestTrueExpr(AllStats.IsEmpty());

				for (auto [ActorClass, Count] : ActorClasses)
				{
					for (int32 Iterations = 0; Iterations < Count; ++Iterations)
					{
						AActor* Actor = World->SpawnActor<AActor>(ActorClass);
						UActorPoolSubsystem::DestroyOrReleaseToPool(WorldContextObject, Actor);
					}

					auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ActorClass);
					TestTrueExpr(Stats.TypeClass == ActorClass);
					TestTrueExpr(Stats.NumberOfPooledObjects == Count);
				}
			});

			It("Should increase the pool size by N, with an existing pool", [this]
			{
				UClass* ActorClass = ATestWorldActor::StaticClass();
				int32 InitialPoolSize = GetRandomValue();
				int32 NewlyInsertedActors = GetRandomValue();

				UActorPoolSubsystem::PopulatePool(WorldContextObject, ActorClass, InitialPoolSize);

				for (int Iteration = 0; Iteration < NewlyInsertedActors; ++Iteration)
				{
					AActor* Actor = World->SpawnActor<AActor>(ActorClass);
					UActorPoolSubsystem::DestroyOrReleaseToPool(WorldContextObject, Actor);
				}

				auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ActorClass);
				TestTrueExpr(Stats.TypeClass == ActorClass);
				TestTrueExpr(Stats.NumberOfPooledObjects == InitialPoolSize+NewlyInsertedActors);
			});

			It("Should increase the pool size by N, with an existing pool, for each pool", [this]
			{
				TArray ActorClasses{
					ClassPair{ATestWorldActor::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Alice::StaticClass(), GetRandomValue()},
					ClassPair{APoolTestActor_Bob::StaticClass(), GetRandomValue()},
				};
				int32 InitialPoolSize = GetRandomValue();

				for (auto [ActorClass, NewlyInsertedActors] : ActorClasses)
				{
					UActorPoolSubsystem::PopulatePool(WorldContextObject, ActorClass, InitialPoolSize);
					for (int Iteration = 0; Iteration < NewlyInsertedActors; ++Iteration)
					{
						AActor* Actor = World->SpawnActor<AActor>(ActorClass);
						UActorPoolSubsystem::DestroyOrReleaseToPool(WorldContextObject, Actor);
					}
				}

				for (auto [ActorClass, NewlyInsertedActors] : ActorClasses)
				{
					auto Stats = UActorPoolSubsystem::GetPoolStats(WorldContextObject, ActorClass);
					TestTrueExpr(Stats.TypeClass == ActorClass);
					TestTrueExpr(Stats.NumberOfPooledObjects == InitialPoolSize + NewlyInsertedActors);
				}
			});
		});

		AfterEach([this]
		{
			World.~FTestWorldHelper();
			UE_LOGFMT(LogObjectPoolingSystemTest, Log, "Test ended.");
		});
	});
}
