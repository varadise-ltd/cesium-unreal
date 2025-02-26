// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/VrdCesium_Common.h"

#include "VrdCesiumCachedMeshLoader.generated.h"

struct FStreamableHandle;
class  UCesiumGltfComponent;
class  AVrdCesium3DTilesetBase;

USTRUCT()
 struct FCesiumLoadCachedMesh_TestParams
{
  GENERATED_BODY()
public:
  UPROPERTY(EditAnywhere) UMeshComponent* meshComp = nullptr;
  UPROPERTY(EditAnywhere) UCesiumGltfComponent* gltfComp = nullptr;
  UPROPERTY(EditAnywhere) UMaterialInstanceDynamic* matInst = nullptr;
 };

USTRUCT()
struct CESIUMRUNTIME_API FVrdCesiumCachedMeshLoader
{
	GENERATED_BODY()
public:
  
  UPROPERTY(EditAnywhere) bool isTestCheckLoadingMesh = false;
  TSet<FSoftObjectPath> _meshpaths;

  UPROPERTY(EditAnywhere) bool isTestSetAllMeshVisible = false;
  UPROPERTY(EditAnywhere) TArray<FCesiumLoadCachedMesh_TestParams> _loadCachedMeshParams;

  UPROPERTY(EditAnywhere) bool isTestTileMeshes = false;
  UPROPERTY(EditAnywhere) UTileMeshes* TileMeshes = nullptr;
  UPROPERTY(EditAnywhere) FString TileMeshesName = "TileMeshes";
  UPROPERTY(EditAnywhere) FString TileMeshesSaveDir = "/Game/Test/cesium";
  UPROPERTY(EditAnywhere) bool isTestSpwanAllToWorld = false;
  UPROPERTY(EditAnywhere) uint32 SpwanAllToWorldCallbackCounter = 0;

  UPROPERTY(EditAnywhere) bool isTestSpwanMeshToWorld = false;
  UPROPERTY(EditAnywhere) FString TestSpwanMeshpath = "/Game/Test/cesium/export/http---192-168-51-91-8084-streams-mtr_tiles-tileset-json/2d456f99-f826-4a88-9964-d00e40c10bb0_glb-mesh-4-primitive-0.2d456f99-f826-4a88-9964-d00e40c10bb0_glb-mesh-4-primitive-0";
  TSharedPtr<FStreamableHandle> TestSpawnMeshHandle;

  UPROPERTY(EditAnywhere) uint32 LoadCachedMeshCbCounter = 0;

  UStaticMesh* LoadCachedMesh(const FString& MeshGamepath, TFunction<void()>&& FnPostLoadMesh, FCesiumLoadCachedMesh_TestParams params);
  void Reset();

  void Tick(float DeltaTime, UWorld* World);

  void AddHandle(const FString& Name, TSharedPtr<FStreamableHandle>& Handle);
  FStreamableHandle* FindHandle(const FString& Name);
  UStaticMesh* FindStaticMesh(const FString& Name);

private:
  TMap<FString, TSharedPtr<FStreamableHandle> > _cachedHandles;
  //TArray<FStreamableHandle*> _pendingCompleteHandles;
};
