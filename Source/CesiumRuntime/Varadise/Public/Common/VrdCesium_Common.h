#pragma once

#include "Cesium3DTilesSelection/Tileset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include <RawMesh.h>
#include <MeshDescription.h>
#include <UObject/SavePackage.h>

#include <Blueprint/WidgetLayoutLibrary.h>
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Components/BillboardComponent.h>

#include <Async/Async.h>
#include <Async/TaskGraphInterfaces.h>

#include "vrdCesium_Common.generated.h"

class Cesium3DTilesSelection::Tile;
class UCesiumGltfComponent;

UCLASS()
class CESIUMRUNTIME_API UCachedTile : public UObject
{
  GENERATED_BODY()
public:
  using CsmTile = Cesium3DTilesSelection::Tile;

public:
  UPROPERTY(VisibleAnywhere) FString name;
  const CsmTile* tile = nullptr;
  UPROPERTY(VisibleAnywhere) UCesiumGltfComponent* gltfComp = nullptr;
  UPROPERTY(VisibleAnywhere) UStaticMeshComponent* meshComp = nullptr;
};

USTRUCT()
struct CESIUMRUNTIME_API FTileMesh
{
	GENERATED_BODY()
public:
  UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMesh>	mesh		= nullptr;
  UPROPERTY(VisibleAnywhere) FTransform					transform;
};

UCLASS()
class CESIUMRUNTIME_API UTileMeshes : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere) TMap<FName, FTileMesh> tileMeshes;

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
    static bool isLeftMouseDown(const UObject* WorldContextObject, int32 PlayerIndex = 0)
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
