// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/VrdCesium_Common.h"

#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"

#include "CesiumRuntime/Public/Cesium3DTileset.h"
#include "CesiumRuntime/Public/CesiumPrimitiveFeatures.h"

#include "../include/Cesium3DTilesSelection/Tileset.h"
#include "cesium-native/Cesium3DTilesSelection/src/TilesetContentManager.h"

//#include "Engine/AssetManager.h"

#include "VrdCesium3DTilesetBase.generated.h"

UENUM()
enum class ESaveUrlToUassetState : uint8 { None, InProgress, Completed, _Count };

struct FStreamableHandle;

class UCesiumGltfComponent;

class AVrdCesium3DTilesetBase;

USTRUCT()
 struct FLoadCachedMesh_TestParams
{
  GENERATED_BODY()
public:
  UPROPERTY(EditAnywhere) UMeshComponent* meshComp = nullptr;
  UPROPERTY(EditAnywhere) UCesiumGltfComponent* gltfComp = nullptr;
  UPROPERTY(EditAnywhere) UMaterialInstanceDynamic* matInst = nullptr;
 };

USTRUCT()
struct CESIUMRUNTIME_API FVrdCachedMeshLoader
{
	GENERATED_BODY()
public:
  
  UPROPERTY(EditAnywhere) bool isTestCheckLoadingMesh = false;
  TSet<FSoftObjectPath> _meshpaths;

  UPROPERTY(EditAnywhere) bool isTestSetAllMeshVisible = false;
  UPROPERTY(EditAnywhere) TArray<FLoadCachedMesh_TestParams> _loadCachedMeshParams;

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


  UStaticMesh* LoadCachedMesh(const FString& MeshGamepath, TFunction<void()>&& FnPostLoadMesh, FLoadCachedMesh_TestParams params);
  void Reset();

  void Tick(float DeltaTime, UWorld* World);

  void AddHandle(const FString& Name, TSharedPtr<FStreamableHandle>& Handle);
  FStreamableHandle* FindHandle(const FString& Name);
  UStaticMesh* FindStaticMesh(const FString& Name);

private:
  TMap<FString, TSharedPtr<FStreamableHandle> > _cachedHandles;
  //TArray<FStreamableHandle*> _pendingCompleteHandles;
};

class FTileList {
  using Tile = Cesium3DTilesSelection::Tile;

public:
  FTileList() = default;

public:
  void clear()
  {
    Tiles.Empty();
    TileSet.Empty();
  }

  void addTile(Tile* tile) {

    Tiles.Add(tile);
    TileSet.Add(tile);

    auto children = tile->getChildren();
    for (auto& child : children) {
      addTile(&child);
    }
  }

  void addTileUnqiue(Tile* tile) {

    if (!TileSet.Find(tile)) {
      Tiles.Add(tile);
      TileSet.Add(tile);
    }

    auto children = tile->getChildren();
    for (auto& child : children) {
      addTileUnqiue(&child);
    }
  }
  
  void addTileUnqiueFromSrc(Tile* tile, const FTileList& TileList) {

    if (!TileList.Find(tile)) {
      Tiles.Add(tile);
      TileSet.Add(tile);
    }

    auto children = tile->getChildren();
    for (auto& child : children) {
      addTileUnqiueFromSrc(&child, TileList);
    }
  }

  void addRange(const TArray<Tile*>& SrcTiles) {
    Tiles.Reserve(SrcTiles.Num());
    TileSet.Reserve(SrcTiles.Num());

    for (auto* tile : SrcTiles) {
      Tiles.Add(tile);
      TileSet.Add(tile);
    }
  }
  
  void addRange(const TArray<Tile*>& SrcTiles, int32 SrcIndex, uint32 Count) {
    
    if (SrcIndex >= SrcTiles.Num()) {
      return;
    }

    Count = FMath::Min(Count, static_cast<uint32>(SrcTiles.Num()) - SrcIndex);

    Tiles.Reserve(Count);
    TileSet.Reserve(Count);
    for (size_t i = SrcIndex; i < SrcIndex + Count; i++) {
      auto* Tile = SrcTiles[i];
      Tiles.Add(Tile);
      TileSet.Add(Tile);
    }
  }

  TArray<Tile*>& getTiles() {
    return this->Tiles;
  }

  int32 Num() const { return this->Tiles.Num(); }
  bool IsEmpty() const { return this->Tiles.IsEmpty(); }

  Tile*const * Find(Tile* tile) const { return TileSet.Find(tile); }

private:
  TArray<Tile*> Tiles;
  TSet<Tile*> TileSet;
};

UENUM()
enum class EVrdTilesetLoadingState : uint8 { None, LoadBatchBegin, LoadBatch, LoadSubBatch, BatchCompleted, OnCompleted, Completed, Failed, _Count };

UENUM()
enum class EVrdTilesetLoaderMode : uint8 { None, SaveTile, Render, _Count };

USTRUCT()
struct CESIUMRUNTIME_API FVrdTilesetLoader
{
	GENERATED_BODY()
public:
  using TilesetContentManager = Cesium3DTilesSelection::TilesetContentManager;
  using Tileset = Cesium3DTilesSelection::Tileset;
  using Tile    = Cesium3DTilesSelection::Tile;
  using RasterOverlayCollection = Cesium3DTilesSelection::RasterOverlayCollection;
  using LoadedLinkedList = Cesium3DTilesSelection::Tile::LoadedLinkedList;
  using TileLoadState = Cesium3DTilesSelection::TileLoadState;

  using LoaderMode = EVrdTilesetLoaderMode;

public:
  UPROPERTY(EditAnywhere) bool bLogInfo = true;

public:
  void RequestLoadTileset(AVrdCesium3DTilesetBase* Tileset, FString Url_, uint32 BatchCount_, float TimesetTimeout_, LoaderMode Mode_);
  void CancelLoadTileset();

  void Tick(float DeltaTime);
  void Reset();

  void LogBatchInfo();

public:
  int32 GetTotalBatchCount() const;
  bool IsCompleted() const;

  int32 GetNumberOfTilesLoaded() const noexcept;

private:
  void _getAllTiles(Tile* RootTile_, Tileset* Tileset_);
  void _loadTileBatchBegin(Tile* RootTile_, Tileset* Tileset_);
  void _loadTileBatchTick(Tile* RootTile_, Tileset* Tileset_);
  void _loadTileBatchEnd(Tile* RootTile_, Tileset* Tileset_);
  void _loadTilesetOnCompleted(Tile* RootTile_, Tileset* Tileset_);
  void _loadTilesetCompleted(Tile* RootTile_, Tileset* Tileset_);

  void unLoadTileListIfLeaf(FTileList& TileList_);
  
private:
  LoadedLinkedList LoadedTiles;

private:
  UPROPERTY()
  AVrdCesium3DTilesetBase* VrdTileset = nullptr;
  CesiumUtility::IntrusivePointer<TilesetContentManager> pTilesetContentManager;

  FTileList TileList;
  int32 LastLoadedTileCount = 0;
  uint32 BatchCount = 32;

  FTileList LoadTileListBatch;
  FTileList LoadTileListSubBatch;
  uint32 LastSubBatchTileCount = 0;

  EVrdTilesetLoaderMode Mode = EVrdTilesetLoaderMode::SaveTile;

  EVrdTilesetLoadingState LoadingState = EVrdTilesetLoadingState::None;
  float TilesetTimeoutTimer = 0.0f;
  float TilesetTimeout = 10.0f;
};

UCLASS()
class CESIUMRUNTIME_API AVrdCesium3DTilesetBase : public ACesium3DTileset 
{
	GENERATED_BODY()
public:
	using CsmTile = Cesium3DTilesSelection::Tile;

public:
  UPROPERTY(EditAnywhere) bool isLoadFromPak = false;		      // temporary
  UPROPERTY(EditAnywhere) UTileMeshes* tileMeshes = nullptr;  // temporary

  UPROPERTY(EditAnywhere, Transient) bool isTestSaveUrlToUasset = false;     // temporary
  UPROPERTY(EditAnywhere, Transient) bool isTestLoadAndSaveAllTiles = false;     // temporary
  UPROPERTY(EditAnywhere) FString SaveUrl = "";                              // temporary

  UPROPERTY(EditAnywhere) FString SaveUrlDir = "";
  UPROPERTY(EditAnywhere, Transient) uint8   IsLoadFromUasset               : 1;

  UPROPERTY(EditAnywhere, Transient) ESaveUrlToUassetState SaveUrlToUassetState = ESaveUrlToUassetState::None;
  //UPROPERTY(EditAnywhere, Transient) uint8   IsSaveUrlToUassetInProgress    : 1;
  //UPROPERTY(EditAnywhere, Transient) uint8   IsSaveUrlToUassetCompleted     : 1;

  UPROPERTY(EditAnywhere) FVrdCachedMeshLoader CachedMeshLoader;
  UPROPERTY() FVrdTilesetLoader TilesetLoader;
  UPROPERTY(EditAnywhere) uint32 LoadBatchCount = 20;
  UPROPERTY(EditAnywhere) float TilesetTimeout = 10.0;

  UPROPERTY(
      EditAnywhere,
      BlueprintGetter = GetForceRenderAllTile,
      BlueprintSetter = SetForceRenderAllTile)
  bool bForceRenderAllTile = false;

public:
  UPROPERTY(EditAnywhere) bool isTestNanite = false;
  UPROPERTY(EditAnywhere) bool isClearTestNanite = false;
  UPROPERTY(EditAnywhere) uint32 testNaniteCount = 0;
  UPROPERTY(EditAnywhere) UMaterialInterface* testNaniteMaterial = nullptr;
  UPROPERTY(EditAnywhere) UStaticMesh* testNaniteMesh = nullptr;
  UPROPERTY(EditAnywhere) TArray<AActor*> testNaniteActors;
  void testNanite();

public:
    static void SetupStaticMesh(bool bIsInit, bool createNavCollision, UStaticMesh* pStaticMesh, UStaticMeshComponent* pMesh, UMaterialInstanceDynamic* pMaterial, UCesiumGltfComponent* pGltf, TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>& pCollisionMesh);
    
public:
	AVrdCesium3DTilesetBase();
  virtual ~AVrdCesium3DTilesetBase();

public:
  UFUNCTION(BlueprintCallable) void ResetSaveUrlToUassetState();
  UFUNCTION(BlueprintCallable) bool SaveUrlToUasset(const FString& InUrl, const FString& InSaveDir);
  UFUNCTION(BlueprintCallable) bool DeleteUrlUasset(const FString& InUrl, const FString& InDir);

  UFUNCTION(BlueprintCallable) bool GetIsSaveUrlToUassetReady() const;
  UFUNCTION(BlueprintCallable) bool GetIsSaveUrlToUassetInProgress() const;
  UFUNCTION(BlueprintCallable) bool GetIsSaveUrlToUassetCompleted() const;

  UFUNCTION() void SaveCachedTilesToUasset();

  UCachedTile* CacheTileStaticMesh(const CesiumGltf::Model& Model, const FString& Name, TUniquePtr<class FStaticMeshRenderData>&& RenderData);
  //UStaticMesh* SaveTileStaticMesh(const CesiumGltf::Model& Model, FStaticMeshRenderData* RenderData);
  void SaveTileToUasset(UCachedTile* Tile, const FString& Outdir, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings, FSavePackageArgs SaveArgs);

  public:
  UFUNCTION(BlueprintCallable, BlueprintGetter) bool GetForceRenderAllTile() const { return bForceRenderAllTile; }
  UFUNCTION(BlueprintCallable, BlueprintSetter) void SetForceRenderAllTile(bool bValue);

public:
  static FString GetMeshUri(const CesiumGltf::Model& Model);
  static FString GetUrlFilename(const FString& InUrl);

  bool IsCachedTileUassetExist(const CesiumGltf::Model& Model, const FString& LoadName) const;

  FString GetCachedTileUassetAbspath(const CesiumGltf::Model& Model, const FString& LoadName) const;
  FString GetCachedTileUassetGamepath(const CesiumGltf::Model& Model, const FString& LoadName) const;
  FString GetMeshName(const FString& Name) const;

public:
  void ClearCacedTiles();
  void ReserveCachedTiles(int32 Num);
  void AddCachedTile(UCachedTile* Value);
  void SetCachedTiles(TArray<UCachedTile*>&& Value);

  void CreateTileMeshes(const FString& AssetName, const FString& OutputDir);

public:
  TArray<UCachedTile*>& GetCachedTiles();
  FString GetExportDirectory() const;

protected:
  virtual void OnDestroyTileset() override;
  virtual void OnLoadTilesetCompleted() override;

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void OnConstruction(const FTransform& transform) override;

protected:	
	UPROPERTY(VisibleAnywhere) TArray<UCachedTile*> _cachedTiles;
};

inline TArray<UCachedTile*>& AVrdCesium3DTilesetBase::GetCachedTiles()
{
  return _cachedTiles;
}


UCLASS()
class CESIUMRUNTIME_API AVrdCesiumSaveUrlsToUassetMonitor : public AActor
{
  GENERATED_BODY()
public:
  UPROPERTY(EditAnywhere)
  TArray<FString> Urls;

  UPROPERTY(EditAnywhere)
  FString SaveDir;
  
  UPROPERTY(EditAnywhere)
  uint32 LoadBatchCount = 20;

  UPROPERTY(EditAnywhere)
  float TilesetTimeout = 10.0f;

  UPROPERTY(EditAnywhere, Transient)
  bool isTestSaveUrlsToUasset = false;     // temporary
  
  UPROPERTY(EditAnywhere, Transient)
  bool isCancelSaveUrlsToUasset = false;     // temporary

  // TODO: remove
  UPROPERTY(EditAnywhere) uint32 LoadCachedMeshCbCounter = 0;


public:
  AVrdCesiumSaveUrlsToUassetMonitor();

public:
  UFUNCTION(BlueprintCallable)
  void RequestSaveUrlsToUasset(const TArray<FString>& InUrls, const FString& InSaveDir, bool IsCloseUnrealAfterCompleted_);

  void CancelSaveUrlsToUasset();

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;

  virtual bool ShouldTickIfViewportsOnly() const override;

protected:
  void SaveUrlsToUassetUpdate(float DeltaTime);

private:
  UPROPERTY(EditAnywhere)
  AVrdCesium3DTilesetBase* Tileset = nullptr;

  uint8 IsCloseUnrealAfterCompleted   : 1;
  uint8 IsSaveUrlsToUassetInProgress  : 1;

  int32 curUrlIndex = 0;

  FVrdTilesetLoader TilesetLoader;
};
