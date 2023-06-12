// Copyright (c) 2023, Hoang Giang Nguyen - Institute for Artificial Intelligence, University Bremen

#include "StateController.h"

#include <chrono>

#include "Engine/StaticMeshActor.h"
#include "Json.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "StateManager.h"
#include "ZMQLibrary/zmq.hpp"

DEFINE_LOG_CATEGORY_STATIC(LogStateController, Log, All);

UStateController::UStateController()
{
    Host = TEXT("tcp://127.0.0.1");
    Port = 7500;
    ColorMap = {
        {FLinearColor(0, 0, 1, 1), TEXT("Blue")},
        {FLinearColor(0, 1, 1, 1), TEXT("Cyan")},
        {FLinearColor(0, 1, 0, 1), TEXT("Green")},
        {FLinearColor(1, 0, 0.5, 1), TEXT("Pink")},
        {FLinearColor(0.5, 0, 1, 1), TEXT("Purple")},
        {FLinearColor(1, 0, 0, 1), TEXT("Red")},
        {FLinearColor(1, 1, 0, 1), TEXT("Yellow")},
        {FLinearColor(0.8, 0.1, 0, 1), TEXT("Orange")},
        {FLinearColor(0.1, 0.1, 0.1, 1), TEXT("Gray")}};
}

UMaterial *UStateController::GetMaterial(const FLinearColor &Color) const
{
	const FString ColorName = TEXT("M_") + ColorMap[Color];
	return Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *(TEXT("Material'/USConnector/Assets/Materials/") + ColorName + TEXT(".") + ColorName + TEXT("'"))));
}

void UStateController::Init()
{
    if (SendObjects.Num() > 0)
    {
        SendObjects.KeySort([](const AActor &ActorA, const AActor &ActorB)
                            { return ActorB.GetName().Compare(ActorA.GetName()) > 0; });
    }
    if (ReceiveObjects.Num() > 0)
    {
        ReceiveObjects.KeySort([](const AActor &ActorA, const AActor &ActorB)
                               { return ActorB.GetName().Compare(ActorA.GetName()) > 0; });
    }

    UWorld *World = GetWorld();
    if (World == nullptr)
    {
        UE_LOG(LogStateController, Error, TEXT("World of %s is nullptr"), *GetName())
        return;
    }

    for (const TPair<AActor *, FAttributeContainer> &ReceiveObject : ReceiveObjects)
    {
        if (ReceiveObject.Key == nullptr)
        {
            UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in ReceiveObjects."))
            continue;
        }

        AStaticMeshActor *StaticMeshActor = Cast<AStaticMeshActor>(ReceiveObject.Key);
        if (StaticMeshActor == nullptr)
        {
            UE_LOG(LogStateController, Warning, TEXT("Ignore Non-StaticMeshActor %s in ReceiveObjects."), *StaticMeshActor->GetName())
            continue;
        }

        UStaticMeshComponent *StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
        if (StaticMeshComponent == nullptr || StaticMeshComponent->GetStaticMesh() == nullptr)
        {
            UE_LOG(LogStateController, Warning, TEXT("StaticMeshActor %s in ReceiveObjects has None StaticMeshComponent."), *ReceiveObject.Key->GetName())
            continue;
        }
        if (!StaticMeshComponent->IsSimulatingPhysics())
        {
            UE_LOG(LogStateController, Warning, TEXT("StaticMeshActor %s has disabled physics, enabling physics."), *ReceiveObject.Key->GetName())
            StaticMeshComponent->SetSimulatePhysics(true);
        }

        UMaterial *RedMaterial = GetMaterial(FLinearColor(1, 0, 0, 1));
        for (int32 i = 0; i < StaticMeshComponent->GetMaterials().Num(); i++)
        {
            StaticMeshComponent->SetMaterial(i, RedMaterial);
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.Template = StaticMeshActor;
        SpawnParams.Name = *(ReceiveObject.Key->GetActorLabel() + TEXT("_ref"));
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor *ReceiveObjectRef = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(), SpawnParams);
        if (ReceiveObjectRef == nullptr)
        {
            UE_LOG(LogStateController, Error, TEXT("Failed to spawn StaticMeshActor %s"), *SpawnParams.Name.ToString())
            continue;
        }

        ReceiveObjectRef->SetActorLabel(SpawnParams.Name.ToString());

        AStaticMeshActor *StaticMeshActorRef = Cast<AStaticMeshActor>(ReceiveObjectRef);

        UStaticMeshComponent *StaticMeshComponentRef = StaticMeshActorRef->GetStaticMeshComponent();
        StaticMeshComponentRef->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        StaticMeshComponentRef->SetSimulatePhysics(false);

        UMaterial *GrayMaterial = GetMaterial(FLinearColor(0.1, 0.1, 0.1, 1));
        for (int32 i = 0; i < StaticMeshComponentRef->GetMaterials().Num(); i++)
        {
            StaticMeshComponentRef->SetMaterial(i, GrayMaterial);
        }

        UPhysicsConstraintComponent *PhysicsConstraint = NewObject<UPhysicsConstraintComponent>(StaticMeshActorRef);
        PhysicsConstraint->AttachToComponent(StaticMeshComponentRef, FAttachmentTransformRules::KeepWorldTransform);
        PhysicsConstraint->SetWorldLocation(StaticMeshActorRef->GetActorLocation());

        PhysicsConstraint->ComponentName1.ComponentName = *StaticMeshActorRef->GetName();
        PhysicsConstraint->ComponentName2.ComponentName = *StaticMeshActor->GetName();
        PhysicsConstraint->ConstraintActor1 = StaticMeshActorRef;
        PhysicsConstraint->ConstraintActor2 = StaticMeshActor;
        PhysicsConstraint->ConstraintInstance.Pos1 = FVector();
        PhysicsConstraint->ConstraintInstance.Pos2 = FVector();

        PhysicsConstraint->SetConstrainedComponents(StaticMeshComponentRef, NAME_None, StaticMeshComponent, NAME_None);

        // Enable linear constraint in all axes
        PhysicsConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
        PhysicsConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.f);
        PhysicsConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.f);

        // Enable angular constraint in all axes
        PhysicsConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
        PhysicsConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0.f);
        PhysicsConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0.f);

        PhysicsConstraint->RegisterComponent();

        ReceiveObjectRefs.Add(ReceiveObjectRef, ReceiveObject.Value);
    }

    if (SendObjects.Num() > 0 || ReceiveObjectRefs.Num() > 0)
    {
        UE_LOG(LogStateController, Log, TEXT("Initializing the socket connection..."))

        context = zmq_ctx_new();

        socket_client = zmq_socket(context, ZMQ_REQ);
        SocketAddr = Host + ":" + FString::FromInt(Port);
        socket_addr = StringCast<ANSICHAR>(*SocketAddr).Get();

        SendMetaData();
    }
}

void UStateController::SendMetaData()
{
    zmq_disconnect(socket_client, socket_addr.c_str());
    zmq_connect(socket_client, socket_addr.c_str());

    SendDataArray.Empty();
    ReceiveDataArray.Empty();

    Task = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
                                                          { const TMap<EAttribute, TArray<double>> AttributeMap =
        {
            {EAttribute::Position, {0.0, 0.0, 0.0}},
            {EAttribute::Quaternion, {1.0, 0.0, 0.0, 0.0}},
            {EAttribute::JointRvalue, {0.0}},
            {EAttribute::JointTvalue, {0.0}},
            {EAttribute::JointPosition, {0.0, 0.0, 0.0}},
            {EAttribute::JointQuaternion, {1.0, 0.0, 0.0, 0.0}}};

        TSharedPtr<FJsonObject> MetaDataJson = MakeShareable(new FJsonObject);
        MetaDataJson->SetStringField("time", "microseconds");
        MetaDataJson->SetStringField("simulator", "unreal");
        MetaDataJson->SetStringField("length_unit", "cm");
        MetaDataJson->SetStringField("angle_unit", "deg");
        MetaDataJson->SetStringField("handedness", "lhs");

        TSharedPtr<FJsonObject> MetaDataSendJson = MakeShareable(new FJsonObject);
        MetaDataJson->SetObjectField("send", MetaDataSendJson);

        send_buffer_size = 1;

        for (const TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
        {
            if (SendObject.Key == nullptr)
            {
                UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in SendObjects"))
                continue;
            }

            TArray<TSharedPtr<FJsonValue>> AttributeJsonArray;
            for (const EAttribute Attribute : SendObject.Value.Attributes)
            {
                switch (Attribute)
                {
                case EAttribute::Position:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("position"))));
                    break;

                case EAttribute::Quaternion:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("quaternion"))));
                    break;

                case EAttribute::JointRvalue:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("joint_rvalue"))));
                    break;

                case EAttribute::JointTvalue:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("joint_tvalue"))));
                    break;

                case EAttribute::JointPosition:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("joint_position"))));
                    break;

                case EAttribute::JointQuaternion:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("joint_quaternion"))));
                    break;

                default:
                    break;
                }

                SendDataArray.Add(TPair<AActor *, EAttribute>(SendObject.Key, Attribute));
                send_buffer_size += AttributeMap[Attribute].Num();
            }
            MetaDataSendJson->SetArrayField(SendObject.Key->GetActorLabel(), AttributeJsonArray);
        }

        TSharedPtr<FJsonObject> MetaDataReceiveJson = MakeShareable(new FJsonObject);
        MetaDataJson->SetObjectField("receive", MetaDataReceiveJson);

        receive_buffer_size = 1;

        for (const TPair<AActor *, FAttributeContainer> &ReceiveObjectRef : ReceiveObjectRefs)
        {
            if (ReceiveObjectRef.Key == nullptr)
            {
                UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in ReceiveObjectRefs"))
                continue;
            }

            AStaticMeshActor *StaticMeshActor = Cast<AStaticMeshActor>(ReceiveObjectRef.Key);
            if (StaticMeshActor == nullptr)
            {
                UE_LOG(LogStateController, Warning, TEXT("Ignore Non-StaticMeshActor %s in ReceiveObjectRefs"), *ReceiveObjectRef.Key->GetName())
                continue;
            }
            else if (StaticMeshActor->GetStaticMeshComponent() == nullptr)
            {
                UE_LOG(LogStateController, Warning, TEXT("StaticMeshActor %s in ReceiveObjectRefs has None StaticMeshComponent"), *ReceiveObjectRef.Key->GetName())
                continue;
            }

            StaticMeshActor->GetStaticMeshComponent()->SetSimulatePhysics(false);

            TArray<TSharedPtr<FJsonValue>> AttributeJsonArray;
            for (const EAttribute &Attribute : ReceiveObjectRef.Value.Attributes)
            {
                switch (Attribute)
                {
                case EAttribute::Position:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("position"))));
                    break;

                case EAttribute::Quaternion:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("quaternion"))));
                    break;

                case EAttribute::JointRvalue:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("joint_rvalue"))));
                    break;

                case EAttribute::JointTvalue:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("joint_tvalue"))));
                    break;

                case EAttribute::JointPosition:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("joint_position"))));
                    break;

                case EAttribute::JointQuaternion:
                    AttributeJsonArray.Add(MakeShareable(new FJsonValueString(TEXT("joint_quaternion"))));
                    break;

                default:
                    break;
                }

                ReceiveDataArray.Add(TPair<AActor *, EAttribute>(ReceiveObjectRef.Key, Attribute));
                receive_buffer_size += AttributeMap[Attribute].Num();
            }

            FString ReceiveObjectName = ReceiveObjectRef.Key->GetActorLabel();
            ReceiveObjectName.RemoveFromEnd(TEXT("_ref"));
            MetaDataReceiveJson->SetArrayField(ReceiveObjectName, AttributeJsonArray);
        }

        FString MetaDataString;
        TSharedRef<TJsonWriter<TCHAR>> Writer = TJsonWriterFactory<TCHAR>::Create(&MetaDataString);
        FJsonSerializer::Serialize(MetaDataJson.ToSharedRef(), Writer, true);

		double *buffer = (double *)calloc(send_buffer_size + 2, sizeof(double));
        std::string meta_data_string;
        for (size_t i = 0; i < (MetaDataString.Len() + 127) / 128; i++) // Split string into multiple substrings with a length of 128 characters or less
        {
            const int32 StartIndex = i * 128;
            const int32 SubstringLength = FMath::Min(128, MetaDataString.Len() - StartIndex);
            const FString Substring = MetaDataString.Mid(StartIndex, SubstringLength);
            meta_data_string += StringCast<ANSICHAR>(*Substring).Get();
        }

        UE_LOG(LogStateController, Log, TEXT("%s"), *MetaDataString)
        
		while (true)
		{
			// Send JSON string over ZMQ
			zmq_send(socket_client, meta_data_string.c_str(), meta_data_string.size(), 0);

			// Receive buffer sizes and send_data (if exists) over ZMQ
			zmq_recv(socket_client, buffer, (send_buffer_size + 2) * sizeof(double), 0);
			if (*buffer < 0)
			{
				free(buffer);
				buffer = (double *)calloc(send_buffer_size + 2, sizeof(double));
				UE_LOG(LogStateController, Warning, TEXT("The socket server at %s has been terminated, resend the message"), *SocketAddr);
				zmq_disconnect(socket_client, socket_addr.c_str());
                zmq_connect(socket_client, socket_addr.c_str());
			}
			else
			{
				break;
			}
		}
        
        size_t recv_buffer_size[2] = {(size_t)buffer[0], (size_t)buffer[1]};
        if (recv_buffer_size[0] != send_buffer_size || recv_buffer_size[1] != receive_buffer_size)
        {
            UE_LOG(LogStateController, Error, TEXT("Failed to initialize the socket at %s: send_buffer_size(server = %ld != client = %ld), receive_buffer_size(server = %ld != client = %ld)."), *SocketAddr, recv_buffer_size[0], send_buffer_size, recv_buffer_size[1], receive_buffer_size)
            zmq_disconnect(socket_client, socket_addr.c_str());
        }
        else
        {
            if (buffer[2] < 0.0)
            {
                UE_LOG(LogStateController, Log, TEXT("Continue state on socket %s"), *SocketAddr)

                double *buffer_addr = buffer + 3;

                for (const TPair<AActor *, FAttributeContainer> &SendObject : SendObjects)
                {
                    if (SendObject.Key == nullptr)
                    {
                        UE_LOG(LogStateController, Warning, TEXT("Ignore None Object in SendObjects"))
                        continue;
                    }

                    TArray<TSharedPtr<FJsonValue>> AttributeJsonArray;
                    for (const EAttribute Attribute : SendObject.Value.Attributes)
                    {
                        switch (Attribute)
                        {
                        case EAttribute::Position:
                        {
                            const float X = *buffer_addr++;
                            const float Y = *buffer_addr++;
                            const float Z = *buffer_addr++;
                            SendObject.Key->SetActorLocation(FVector(X, Y, Z));
                            break;
                        }

                        case EAttribute::Quaternion:
                        {
                            const float W = *buffer_addr++;
                            const float X = *buffer_addr++;
                            const float Y = *buffer_addr++;
                            const float Z = *buffer_addr++;
                            SendObject.Key->SetActorRotation(FQuat(X, Y, Z, W));
                            break;
                        }

                        case EAttribute::JointPosition:
                        {
                            break;
                        }

                        case EAttribute::JointQuaternion:
                        {
                            break;
                        }

                        default:
                            break;
                        }
                    }
                }
            }

            UE_LOG(LogStateController, Log, TEXT("Initialized the socket at %s successfully."), *SocketAddr)
            UE_LOG(LogStateController, Log, TEXT("Start communication on %s (send: %ld, receive: %ld)"), *SocketAddr, send_buffer_size, receive_buffer_size)
            send_buffer = (double *)calloc(send_buffer_size, sizeof(double));
            receive_buffer = (double *)calloc(receive_buffer_size, sizeof(double));
            IsEnable = true;
        } },
                                                          TStatId(), nullptr, ENamedThreads::AnyThread);
}

void UStateController::Tick()
{
    if (IsEnable)
    {
        *send_buffer = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

        double *send_buffer_addr = send_buffer + 1;
        for (TPair<AActor *, EAttribute> &SendData : SendDataArray)
        {
            switch (SendData.Value)
            {
            case EAttribute::Position:
            {
                const FVector ActorLocation = SendData.Key->GetActorLocation();
                *send_buffer_addr++ = ActorLocation.X;
                *send_buffer_addr++ = ActorLocation.Y;
                *send_buffer_addr++ = ActorLocation.Z;
                break;
            }

            case EAttribute::Quaternion:
            {
                const FQuat ActorQuat = SendData.Key->GetActorQuat();
                *send_buffer_addr++ = ActorQuat.W;
                *send_buffer_addr++ = ActorQuat.X;
                *send_buffer_addr++ = ActorQuat.Y;
                *send_buffer_addr++ = ActorQuat.Z;
                break;
            }

            default:
                break;
            }
        }

        zmq_send(socket_client, send_buffer, send_buffer_size * sizeof(double), 0);

        zmq_recv(socket_client, receive_buffer, receive_buffer_size * sizeof(double), 0);

        if (*receive_buffer < 0)
        {
            IsEnable = false;
            UE_LOG(LogStateController, Warning, TEXT("The socket server at %s has been terminated, resend the message"), *SocketAddr)
            if (Task.IsValid())
            {
                Task->Wait();
            }
            SendMetaData();
            return;
        }

        double *receive_buffer_addr = receive_buffer + 1;
        for (TPair<AActor *, EAttribute> &ReceiveData : ReceiveDataArray)
        {
            switch (ReceiveData.Value)
            {
            case EAttribute::Position:
            {
                const float X = *receive_buffer_addr++;
                const float Y = *receive_buffer_addr++;
                const float Z = *receive_buffer_addr++;
                ReceiveData.Key->SetActorLocation(FVector(X, Y, Z));
                break;
            }

            case EAttribute::Quaternion:
            {
                const float W = *receive_buffer_addr++;
                const float X = *receive_buffer_addr++;
                const float Y = *receive_buffer_addr++;
                const float Z = *receive_buffer_addr++;
                ReceiveData.Key->SetActorRotation(FQuat(X, Y, Z, W));
                break;
            }

            default:
                break;
            }
        }
    }
}

void UStateController::Deinit()
{
    UE_LOG(LogStateController, Log, TEXT("Closing the socket client on %s"), *SocketAddr);
    if (IsEnable)
    {
        const std::string close_data = "{}";

        zmq_send(socket_client, close_data.c_str(), close_data.size(), 0);

        free(send_buffer);
        free(receive_buffer);

        zmq_disconnect(socket_client, socket_addr.c_str());
    }
    else if (Task.IsValid())
    {
        zmq_ctx_shutdown(context);
        Task->Wait();
    }
}