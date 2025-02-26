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
      case EVrdTilesetLoadingState::None:             { _getAllTiles(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::LoadBatchBegin:   { _loadTileBatchBegin(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::LoadBatch:        { _loadTileBatchTick(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::LoadSubBatch:     { _loadTileBatchTick(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::BatchCompleted:   { _loadTileBatchEnd(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::OnCompleted:      { _loadTilesetOnCompleted(rootTile, rootTileset); } break;
      case EVrdTilesetLoadingState::Completed:        { _loadTilesetCompleted(rootTile, rootTileset); } break;
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

  TileList.clear();
  LastLoadedTileCount = 0;
  BatchCount = 0;

  LoadTileListBatch.clear();
  LoadTileListSubBatch.clear();
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

void FVrdCesiumTilesetLoader::_getAllTiles(Tile* Tile_, Tileset* Tileset_)
{
  if (!pTilesetContentManager)
  {
    TileList.addTile(Tile_);
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

void FVrdCesiumTilesetLoader::_loadTileBatchBegin(Tile* RootTile_, Tileset* Tileset_)
{
  LoadTileListBatch.addRange(TileList.getTiles(), LastLoadedTileCount, BatchCount);
  LoadingState = EVrdTilesetLoadingState::LoadBatch;
}

void FVrdCesiumTilesetLoader::_loadTileBatchTick(Tile* RootTile_, Tileset* Tileset_)
{
  uint32 loadedBatchCount = 0;
  bool  isLoadBatch   = LoadingState == EVrdTilesetLoadingState::LoadBatch;
  auto* TileListBatch = isLoadBatch ? &LoadTileListBatch : &LoadTileListSubBatch;

  bool isCompleted = true;
  for (auto* pTile : TileListBatch->getTiles())
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

    // addTileToLoadQueue
    if (pTilesetContentManager->tileNeedsWorkerThreadLoading(Tile_))
    {
      pTilesetContentManager->loadTileContent(Tile_, Tileset_->getOptions());    // _processWorkerThreadLoadQueue
    }
    else if (pTilesetContentManager->tileNeedsMainThreadLoading(Tile_))
    {
      pTilesetContentManager->finishLoading(Tile_, Tileset_->getOptions());      // _processMainThreadLoadQueue

      // finishLoading will call updateTileContent(tile, tilesetOptions), this may create new tiles
      LoadTileListSubBatch.addTileUnqiueFromSrc(pTile, TileList);
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

void FVrdCesiumTilesetLoader::_loadTileBatchEnd(Tile* RootTile_, Tileset* Tileset_)
{
  if (Mode == EVrdTilesetLoaderMode::SaveTile)
  {
    VrdTileset->SaveCachedTilesToUasset();
    VrdTileset->ClearCachedTiles();

    // if unload parent, if may affect the flatten tileList
    unLoadTileListIfLeaf(LoadTileListBatch);
    unLoadTileListIfLeaf(LoadTileListSubBatch);
  }
  else if (Mode == EVrdTilesetLoaderMode::Render)
  {
    check(VrdTileset);

    std::vector<Tile*> Tiles;
    ToStdVector_Copy(Tiles, LoadTileListBatch.getTiles());
    //VrdTileset->updateLastViewUpdateResultState()
    VrdTileset->showTilesToRender(Tiles);

    ToStdVector_Copy(Tiles, LoadTileListSubBatch.getTiles());
    VrdTileset->showTilesToRender(Tiles);

    LoadTileListBatch.clear();
    LoadTileListSubBatch.clear();
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

void FVrdCesiumTilesetLoader::_loadTilesetOnCompleted(Tile* RootTile_, Tileset* Tileset_)
{
  LogBatchInfo();
  LoadingState = EVrdTilesetLoadingState::Completed;
}

void FVrdCesiumTilesetLoader::_loadTilesetCompleted(Tile* RootTile_, Tileset* Tileset_)
{
  
}

void FVrdCesiumTilesetLoader::unLoadTileListIfLeaf(FCesiumTileList& TileList_)
{
  for (auto* Tile : TileList_.getTiles())
  {
    bool bIsLeaf = Tile->getChildren().empty();
    if (bIsLeaf)
    {
      pTilesetContentManager->unloadTileContent(*Tile);
    }
  }
  TileList_.clear();
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
