#include "VrdCesiumTilesetLoader.h"

#include "Components/StaticMeshComponent.h"
#include "MaterialDomain.h"

#include "CesiumRuntime/Private/CesiumGltfComponent.h"
#include "CesiumRuntime/Private/VecMath.h"
#include "CesiumRuntime/Private/CesiumPrimitive.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Components/BillboardComponent.h"
#include "Engine/AssetManager.h"

void FVrdCesiumTilesetLoader::RequestLoadTileset(AVrdCesium3DTilesetBase* Tileset, FString Url_, uint32 BatchCount_, float TimesetTimeout_, LoaderMode Mode_)
{
  Reset();
  VrdTileset = Tileset;
  BatchCount = BatchCount_;
  LoadingState = EVrdTilesetLoadingState::None;
  TilesetTimeout = TimesetTimeout_;

  Mode = Mode_;
  // FGenericPlatformProcess::ConditionalSleep(fnIsSaveCompleted, 0.1f);*/
}

void FVrdCesiumTilesetLoader::CancelLoadTileset()
{
  if (VrdTileset)
  {
    VrdTileset->ResetSaveUrlToUassetState();
  }
  Reset();
}

void FVrdCesiumTilesetLoader::Tick(float DeltaTime)
{
  if (!VrdTileset)
  {
    return;
  }

  auto* rootTileset = VrdTileset->GetTileset();
  if (!rootTileset) {
    return;
  }
   rootTileset->getAsyncSystem().dispatchMainThreadTasks();
  auto* rootTile = rootTileset->getRootTile();

  bool isTilesetReady = rootTile != nullptr;
  if (isTilesetReady)
  {
    switch (LoadingState)
    {
      case EVrdTilesetLoadingState::None:             { _GetAllTiles(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::LoadBatchBegin:   { _LoadTileBatchBegin(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::LoadBatch:        { _LoadTileBatchTick(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::LoadSubBatch:     { _LoadTileBatchTick(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::BatchCompleted:   { _LoadTileBatchEnd(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::OnCompleted:      { _LoadTilesetOnCompleted(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::Completed:        { _LoadTilesetCompleted(rootTile, rootTileset); } break;
      default: {} break;
    }
  }
  else
  {
    TilesetTimeoutTimer += DeltaTime;
    if (TilesetTimeoutTimer >= TilesetTimeout)
    {
      LoadingState = EVrdTilesetLoadingState::Failed;
    }
  }
  
}

void FVrdCesiumTilesetLoader::Reset()
{
  VrdTileset = nullptr;
  pTilesetContentManager.reset();

  TileList.Clear();
  LastLoadedTileCount = 0;
  BatchCount = 0;

  LoadTileListBatch.Clear();
  LoadTileListSubBatch.Clear();
  LastSubBatchTileCount = 0;

  LoadingState = EVrdTilesetLoadingState::None;

  TilesetTimeout = 0.0f;
  TilesetTimeoutTimer = 0.0f;
}

void FVrdCesiumTilesetLoader::LogBatchInfo()
{
  if (!bLogInfo)
  {
    return;
  }

  UE_LOG(LogTemp, Warning, TEXT("FVrdCesiumTilesetLoader::GetNumberOfTilesLoaded: %d"), GetNumberOfTilesLoaded());
  UE_LOG(LogTemp, Warning, TEXT("LoadedCount: %d / %d"), LastLoadedTileCount, TileList.Num());
}

void FVrdCesiumTilesetLoader::_GetAllTiles(Tile* Tile_, Tileset* Tileset_)
{
  if (!pTilesetContentManager)
  {
    TileList.AddTile(Tile_);
    UE_LOG(LogTemp, Warning, TEXT("TileList.Num(): %d"), TileList.Num());

    pTilesetContentManager =
                  new TilesetContentManager(
                      Tileset_->getExternals(),
                      Tileset_->getOptions(),
                      RasterOverlayCollection{
                          LoadedTiles,
                          Tileset_->getExternals(),
                          Tileset_->getOptions().ellipsoid},
                       std::string(TCHAR_TO_UTF8(*VrdTileset->GetUrl())));
  }
  
  bool isContentManagerReady = pTilesetContentManager && pTilesetContentManager->getRootTile();
  if (isContentManagerReady)
  {
    LoadingState = EVrdTilesetLoadingState::LoadBatchBegin;
  }
}

void FVrdCesiumTilesetLoader::_LoadTileBatchBegin(Tile* RootTile_, Tileset* Tileset_)
{
  LoadTileListBatch.AddRange(TileList.GetTiles(), LastLoadedTileCount, BatchCount);
  LoadingState = EVrdTilesetLoadingState::LoadBatch;
}

void FVrdCesiumTilesetLoader::_LoadTileBatchTick(Tile* RootTile_, Tileset* Tileset_)
{
  uint32 loadedBatchCount = 0;
  bool  isLoadBatch   = LoadingState == EVrdTilesetLoadingState::LoadBatch;
  auto* TileListBatch = isLoadBatch ? &LoadTileListBatch : &LoadTileListSubBatch;

  bool isCompleted = true;
  for (auto* pTile : TileListBatch->GetTiles())
  {
    auto& Tile_ = *pTile;
    auto TileState = Tile_.getState();
    bool isTileLoaded = (
                        TileState == TileLoadState::Done
                        || (TileState == TileLoadState::ContentLoaded && !Tile_.getContent().isRenderContent())
                        || (TileState == TileLoadState::FailedTemporarily)
                        || (TileState == TileLoadState::Failed)
    );

    isCompleted &= isTileLoaded;
    if (isTileLoaded)
    {
      continue;
    }

    // AddTileToLoadQueue
    if (pTilesetContentManager->tileNeedsWorkerThreadLoading(Tile_))
    {
      pTilesetContentManager->loadTileContent(Tile_, Tileset_->getOptions());    // _processWorkerThreadLoadQueue
    }
    else if (pTilesetContentManager->tileNeedsMainThreadLoading(Tile_))
    {
      pTilesetContentManager->finishLoading(Tile_, Tileset_->getOptions());      // _processMainThreadLoadQueue

      // finishLoading will call updateTileContent(tile, tilesetOptions), this may create new tiles
      LoadTileListSubBatch.AddTileUnqiueFromSrc(pTile, TileList);
    }
  }

  //bool isCompleted = (loadedBatchCount == BatchCount && ) || ();
  if (isCompleted)
  {
    LoadingState = EVrdTilesetLoadingState::BatchCompleted;
    if (isLoadBatch)
    {
      LastLoadedTileCount += LoadTileListBatch.Num();
    }

    auto CurSubBatchTileCount = LoadTileListSubBatch.Num();
    if (LastSubBatchTileCount != CurSubBatchTileCount)
    {
      LoadingState = EVrdTilesetLoadingState::LoadSubBatch;
    }
    LastSubBatchTileCount = CurSubBatchTileCount;
  }
}

template<class T>
void ToStdVector_Copy(std::vector<T>& Out, const TArray<T>& Data)
{
  Out.clear();
  Out.reserve(Data.Num());
  for (auto& e : Data)
  {
    Out.emplace_back(e);
  }
}

void FVrdCesiumTilesetLoader::_LoadTileBatchEnd(Tile* RootTile_, Tileset* Tileset_)
{
  if (Mode == EVrdTilesetLoaderMode::SaveTile)
  {
    VrdTileset->SaveCachedTilesToUasset();
    VrdTileset->ClearCachedTiles();

    // if unload parent, if may affect the flatten tileList
    UnLoadTileListIfLeaf(LoadTileListBatch);
    UnLoadTileListIfLeaf(LoadTileListSubBatch);
  }
  else if (Mode == EVrdTilesetLoaderMode::Render)
  {
    check(VrdTileset);

    std::vector<Tile*> Tiles;
    ToStdVector_Copy(Tiles, LoadTileListBatch.GetTiles());
    //VrdTileset->updateLastViewUpdateResultState()
    VrdTileset->showTilesToRender(Tiles);

    ToStdVector_Copy(Tiles, LoadTileListSubBatch.GetTiles());
    VrdTileset->showTilesToRender(Tiles);

    LoadTileListBatch.Clear();
    LoadTileListSubBatch.Clear();
  }

  if (LastLoadedTileCount >= TileList.Num())
  {
    LoadingState = EVrdTilesetLoadingState::OnCompleted;
  } else
  {
    LoadingState = EVrdTilesetLoadingState::LoadBatchBegin;
  }

  LogBatchInfo();
}

void FVrdCesiumTilesetLoader::_LoadTilesetOnCompleted(Tile* RootTile_, Tileset* Tileset_)
{
  LogBatchInfo();
  LoadingState = EVrdTilesetLoadingState::Completed;
}

void FVrdCesiumTilesetLoader::_LoadTilesetCompleted(Tile* RootTile_, Tileset* Tileset_)
{
  
}

void FVrdCesiumTilesetLoader::UnLoadTileListIfLeaf(FCesiumTileList& TileList_)
{
  for (auto* Tile : TileList_.GetTiles())
  {
    bool bIsLeaf = Tile->getChildren().empty();
    if (bIsLeaf)
    {
      pTilesetContentManager->unloadTileContent(*Tile);
    }
  }
  TileList_.Clear();
}

int32 FVrdCesiumTilesetLoader::GetTotalBatchCount() const { return FMath::CeilToInt(TileList.Num() / (float)BatchCount); }

bool FVrdCesiumTilesetLoader::IsCompleted() const {
  return LoadingState == EVrdTilesetLoadingState::Completed ||
         LoadingState == EVrdTilesetLoadingState::Failed;
}

int32 FVrdCesiumTilesetLoader::GetNumberOfTilesLoaded() const noexcept
{
  return VrdTileset && VrdTileset->GetTileset() ? VrdTileset->GetTileset()->getNumberOfTilesLoaded() : 0;
}
