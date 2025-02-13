// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/VrdCesium_Common.h"
#include "CesiumRuntime/Public/Cesium3DTileset.h"
#include "CesiumRuntime/Public/CesiumPrimitiveFeatures.h"

#include "../include/Cesium3DTilesSelection/Tileset.h"
#include "cesium-native/Cesium3DTilesSelection/src/TilesetContentManager.h"

#include "VrdCesium3DTilesetBase.generated.h"

UENUM()
enum class ESaveUrlToUassetState : uint8 { None, InProgress, Completed, _Count };

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

  UPROPERTY(EditAnywhere) bool    bIsTickInEditor = false;     // temporary
  UPROPERTY(EditAnywhere) FString SaveUrlDir = "";
  UPROPERTY(EditAnywhere, Transient) uint8   IsLoadFromUasset               : 1;

  UPROPERTY(EditAnywhere, Transient) ESaveUrlToUassetState SaveUrlToUassetState = ESaveUrlToUassetState::None;
  //UPROPERTY(EditAnywhere, Transient) uint8   IsSaveUrlToUassetInProgress    : 1;
  //UPROPERTY(EditAnywhere, Transient) uint8   IsSaveUrlToUassetCompleted     : 1;

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
  static FString GetMeshUri(const CesiumGltf::Model& Model);
  static FString GetUrlFilename(const FString& InUrl);

  bool IsCachedTileUassetExist(const CesiumGltf::Model& Model, const FString& LoadName) const;

  FString GetCachedTileUassetAbspath(const CesiumGltf::Model& Model, const FString& LoadName) const;
  FString GetCachedTileUassetGamepath(const CesiumGltf::Model& Model, const FString& LoadName) const;
  FString GetMeshName(const FString& Name) const;

public:
  void ReserveCachedTiles(int32 Num);
  void AddCachedTile(UCachedTile* Value);
  void SetCachedTiles(TArray<UCachedTile*>&& Value);

  void CreateTileMeshes(const FString& AssetName, const FString& OutputDir);

  void ClearCacedTiles();

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
enum class EVrdTilesetLoadingState : uint8 { None, LoadBatchBegin, LoadBatch, LoadSubBatch, BatchCompleted, Completed, Failed, _Count };

class FVrdTilesetLoader
{
public:
  using TilesetContentManager = Cesium3DTilesSelection::TilesetContentManager;
  using Tileset = Cesium3DTilesSelection::Tileset;
  using Tile    = Cesium3DTilesSelection::Tile;
  using RasterOverlayCollection = Cesium3DTilesSelection::RasterOverlayCollection;
  using LoadedLinkedList = Cesium3DTilesSelection::Tile::LoadedLinkedList;
  using TileLoadState = Cesium3DTilesSelection::TileLoadState;

public:
  void RequestLoadTileset(AVrdCesium3DTilesetBase* Tileset, FString Url_, uint32 BatchCount_, float TimesetTimeout_);
  void CancelLoadTileset();

  void Tick(float DeltaTime);
  void Reset();

  int32 GetTotalBatchCount() const;
  bool IsCompleted() const;

private:
  void _getAllTiles(Tile* RootTile_, Tileset* Tileset_);
  void _loadTileBatchBegin(Tile* RootTile_, Tileset* Tileset_);
  void _loadTileBatchTick(Tile* RootTile_, Tileset* Tileset_);
  void _loadTileBatchEnd(Tile* RootTile_, Tileset* Tileset_);
  void _loadTilesetCompleted(Tile* RootTile_, Tileset* Tileset_);

  void unLoadTileListIfLeaf(FTileList& TileList_);
  
private:
  LoadedLinkedList LoadedTiles;

private:
  AVrdCesium3DTilesetBase* VrdTileset = nullptr;
  CesiumUtility::IntrusivePointer<TilesetContentManager> pTilesetContentManager;

  FTileList TileList;
  int32 LastLoadedTileCount = 0;
  uint32 BatchCount = 20;

  FTileList LoadTileListBatch;
  FTileList LoadTileListSubBatch;
  uint32 LastSubBatchTileCount = 0;

  EVrdTilesetLoadingState LoadingState = EVrdTilesetLoadingState::None;
  float TilesetTimeoutTimer = 0.0f;
  float TilesetTimeout = 10.0f;
};

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
