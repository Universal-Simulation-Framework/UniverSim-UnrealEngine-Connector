// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#pragma once

#include "CoreMinimal.h"

// clang-format off
#include "MultiverseClientComponent.generated.h"
// clang-format on

struct FAttributeContainer;
class FMultiverseClient;

UCLASS(Blueprintable, DefaultToInstanced, collapsecategories, hidecategories = Object, editinlinenew)
class MULTIVERSECONNECTOR_API UMultiverseClientComponent final : public UObject
{
	GENERATED_BODY()

public:
	UMultiverseClientComponent();

public:
	void Init();

	void Tick();

	void Deinit();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerHost = TEXT("tcp://127.0.0.1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerPort = TEXT("7000");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ClientHost = TEXT("tcp://127.0.0.1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ClientPort = TEXT("9000");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WorldName = TEXT("world");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SimulationName = TEXT("unreal");

	UPROPERTY(EditAnywhere)
	TMap<AActor *, FAttributeContainer> SendObjects;

	UPROPERTY(EditAnywhere)
	TMap<AActor *, FAttributeContainer> ReceiveObjects;

private:
	FMultiverseClient MultiverseClient;
};