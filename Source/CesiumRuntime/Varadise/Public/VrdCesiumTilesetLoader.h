// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/VrdCesium_Common.h"
#include "VrdCesiumTileList.h"

#include "../include/Cesium3DTilesSelection/Tileset.h"
#include "cesium-native/Cesium3DTilesSelection/src/TilesetContentManager.h"

#include "VrdCesiumTilesetLoader.generated.h"

class AVrdCesium3DTilesetBase;

UENUM()
enum class EVrdTilesetLoadingState : uint8
{
  None,
  LoadBatchBegin,
  LoadBatch,
  LoadSubBatch,
  BatchCompleted,
  OnCompleted,
  Completed,
  Failed,
  _Count
};

UENUM()
enum class EVrdTilesetLoaderMode : uint8
{
  None,
  SaveTile,
  Render,
  _Count
};

USTRUCT()
struct CESIUMRUNTIME_API FVrdCesiumTilesetLoader
{
	GENERATED_BODY()
public:
  using TilesetContentManager   = Cesium3DTilesSelection::TilesetContentManager;
  using Tileset                 = Cesium3DTilesSelection::Tileset;
  using Tile                    = Cesium3DTilesSelection::Tile;
  using RasterOverlayCollection = Cesium3DTilesSelection::RasterOverlayCollection;
  using LoadedLinkedList        = Cesium3DTilesSelection::Tile::LoadedLinkedList;
  using TileLoadState           = Cesium3DTilesSelection::TileLoadState;

  using LoaderMode              = EVrdTilesetLoaderMode;

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
  void _GetAllTiles(Tile* RootTile_, Tileset* Tileset_);
  void _LoadTileBatchBegin(Tile* RootTile_, Tileset* Tileset_);
  void _LoadTileBatchTick(Tile* RootTile_, Tileset* Tileset_);
  void _LoadTileBatchEnd(Tile* RootTile_, Tileset* Tileset_);
  void _LoadTilesetOnCompleted(Tile* RootTile_, Tileset* Tileset_);
  void _LoadTilesetCompleted(Tile* RootTile_, Tileset* Tileset_);

  void UnLoadTileListIfLeaf(FCesiumTileList& TileList_);
  
private:
  LoadedLinkedList LoadedTiles;

private:
  UPROPERTY()
  AVrdCesium3DTilesetBase* VrdTileset = nullptr;
  CesiumUtility::IntrusivePointer<TilesetContentManager> pTilesetContentManager;

  FCesiumTileList TileList;
  int32 LastLoadedTileCount = 0;
  uint32 BatchCount = 32;

  FCesiumTileList LoadTileListBatch;
  FCesiumTileList LoadTileListSubBatch;
  uint32 LastSubBatchTileCount = 0;

  EVrdTilesetLoaderMode Mode = EVrdTilesetLoaderMode::SaveTile;

  EVrdTilesetLoadingState LoadingState = EVrdTilesetLoadingState::None;
  float TilesetTimeoutTimer = 0.0f;
  float TilesetTimeout = 10.0f;
};
