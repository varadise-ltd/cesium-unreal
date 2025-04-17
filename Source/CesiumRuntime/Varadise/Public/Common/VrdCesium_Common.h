#pragma once

#include "Cesium3DTilesSelection/Tileset.h"

#include <UObject/SavePackage.h>

#include <Blueprint/WidgetLayoutLibrary.h>
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Components/BillboardComponent.h>

#include <PhysicsEngine/BodySetup.h>
#include <PhysicsEngine/PhysicsSettings.h>

#include <Async/Async.h>
#include <Async/TaskGraphInterfaces.h>

#include <Components/StaticMeshComponent.h>
#include <RawMesh.h>
#include <MeshDescription.h>

#include <MaterialDomain.h>
#include <Materials/MaterialInstanceDynamic.h>

#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetSystemLibrary.h>

#include <Editor/UnrealEd/Public/EditorViewportClient.h>
#include <Editor/UnrealEd/Public/LevelEditorViewport.h>

#include "vrdCesium_Common.generated.h"

#define VRD_CESIUM_DEBUG 0

#ifndef CESIUMRUNTIME_API
  #define CESIUMRUNTIME_API
#endif // undef CESIUMRUNTIME_API

class Cesium3DTilesSelection::Tile;
class UCesiumGltfComponent;

/*
* cache cesium (csm) tile for furter process
* probably legacy
*/
UCLASS()
class CESIUMRUNTIME_API UCachedTile : public UObject
{
  GENERATED_BODY()
public:
  using CsmTile = Cesium3DTilesSelection::Tile;

public:
  const CsmTile* Tile = nullptr;

  UPROPERTY(VisibleAnywhere)
  FString Name;

  UPROPERTY(VisibleAnywhere)
  UCesiumGltfComponent* GltfComp = nullptr;

  UPROPERTY(VisibleAnywhere)
  UStaticMeshComponent* MeshComp = nullptr;
};

/*
* save transform info and its corresponding mesh 
* probably legacy
*/
USTRUCT()
struct CESIUMRUNTIME_API FTileMesh
{
	GENERATED_BODY()
public:
  UPROPERTY(VisibleAnywhere)
  TObjectPtr<UStaticMesh>	Mesh = nullptr;

  UPROPERTY(VisibleAnywhere)
  FTransform Transform;
};

/*
* spwan all tile set with the transform by the loaded name, for debug purpose
*/
UCLASS()
class CESIUMRUNTIME_API UTileMeshes : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere)
  TMap<FName, FTileMesh> tileMeshes;

public:
  static AActor* SpwanMeshActor(UWorld* World, UStaticMesh* Mesh, const FTransform& Transform);
  static void AddMeshComponent(AActor* Actor, UStaticMesh* Mesh, const FTransform& Transform);

public:
	void SpwanAllToWorld(UWorld* World);
};

struct CESIUMRUNTIME_API MeshUtil
{
public:
    static void GetRawMeshTo(FRawMesh& Out, const FStaticMeshVertexBuffers& VtxBufs, const FRawStaticIndexBuffer& IdxBuf);

    static UStaticMesh* DuplicateStaticMesh(UStaticMesh* SourceMesh, const FString& Name, UObject* Owner = (UObject*)GetTransientPackage(), EObjectFlags ObjectFlags = RF_Public | RF_Standalone);

    static void StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, FStaticMeshRenderData* Src, const TArray<FStaticMaterial>& StaticMaterials, const FMeshBuildSettings& BuildSettings = {}, const FMeshNaniteSettings& NaniteSettings = {});
    static void StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, FStaticMeshRenderData* Src, const FMeshBuildSettings& BuildSettings = {}, const FMeshNaniteSettings& NaniteSettings = {});
    static void StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, UStaticMesh* Src, const TArray<FStaticMaterial>& StaticMaterials, const FMeshBuildSettings& BuildSettings = {}, const FMeshNaniteSettings& NaniteSettings = {});
};

struct CESIUMRUNTIME_API AssetUtil
{
public:
  static UPackage* SaveUObject(UObject* UObj, const FString& AssetName, const FString& PackagePath, FSavePackageArgs SaveArgs);
  static UPackage* SaveUObject(UObject* UObj, const FString& AssetName, const FString& PackagePath);
};

struct CESIUMRUNTIME_API PathUtil
{
public:
    static bool IsFileExist(const FString& Filepath)
    {
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        return PlatformFile.FileExists(*Filepath);
    }
};

struct CESIUMRUNTIME_API InputUtil
{
    static bool IsLeftMouseDown(const UObject* WorldContextObject, int32 PlayerIndex = 0)
    {
        if (!WorldContextObject)
        {
            return false;
        }

	    auto* playerCtrl = UGameplayStatics::GetPlayerController(WorldContextObject, PlayerIndex);
        if (!playerCtrl) {
              return false;
        }

        return playerCtrl->IsInputKeyDown(EKeys::LeftMouseButton);
    }
};

inline bool DeprojectMousePosToWorldByEditorViewport(FVector& outWorldPos, FVector& outWorldDir)
{
	//auto* client = Cast<FEditorViewportClient>(GEditor->GetActiveViewport()->GetClient());
	auto* client = GLastKeyLevelEditingViewportClient;
	if (!client)
		return false;
	
	FIntPoint mousePos;
	mousePos.X = client->GetCachedMouseX();
	mousePos.Y = client->GetCachedMouseY();

	FSceneViewFamily viewFamily = FSceneViewFamily::ConstructionValues(client->Viewport, client->GetScene(), client->EngineShowFlags);
	FSceneView* culScene = client->CalcSceneView(&viewFamily);

	culScene->DeprojectFVector2D(mousePos, outWorldPos, outWorldDir);
	return true;
}

inline void LineTrace_Editor(FHitResult& OutHit, UWorld* world)
{
	check(world);
	FVector worldPos, worldDir;
	bool isSuccess = DeprojectMousePosToWorldByEditorViewport(worldPos, worldDir);
	if (!isSuccess)
		return;

	TArray<TEnumAsByte<EObjectTypeQuery>> objectTypes;
	objectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	FHitResult& hit = OutHit;
	UKismetSystemLibrary::LineTraceSingleForObjects(world, worldPos, worldPos + worldDir * 9999.0f, objectTypes, true, {}, EDrawDebugTrace::None, hit, true);
}
