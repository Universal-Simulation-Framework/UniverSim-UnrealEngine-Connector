// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "MultiverseClientComponent.h"

#include "MultiverseClient.h"

UMultiverseClientComponent::UMultiverseClientComponent()
{
    ServerHost = TEXT("tcp://127.0.0.1");
    ServerPort = TEXT("7000");
    ClientPort = TEXT("7600");
}

void UMultiverseClientComponent::Init()
{   
    MultiverseClient.Init(ServerHost, ServerPort, ClientPort, WorldName, SimulationName, SendObjects, ReceiveObjects, GetWorld());
}

void UMultiverseClientComponent::Tick()
{
    MultiverseClient.communicate();
}

void UMultiverseClientComponent::Deinit()
{
    MultiverseClient.disconnect();
}