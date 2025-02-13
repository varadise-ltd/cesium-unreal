#include "VrdCesium3DTilesetBase.h"

#include "Components/StaticMeshComponent.h"

#include "CesiumRuntime/Private/CesiumGltfComponent.h"
#include "CesiumRuntime/Private/VecMath.h"
#include "CesiumRuntime/Private/CesiumPrimitive.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Components/BillboardComponent.h"
#include "Engine/AssetManager.h"

AVrdCesium3DTilesetBase::AVrdCesium3DTilesetBase()
	: Super()
{
  IsLoadFromUasset              = false;
  SaveUrlToUassetState          = ESaveUrlToUassetState::None; 
  //IsSaveUrlToUassetInProgress   = false;
  //IsSaveUrlToUassetCompleted    = false;
}

AVrdCesium3DTilesetBase::~AVrdCesium3DTilesetBase()
{
	
}

void AVrdCesium3DTilesetBase::OnDestroyTileset()
{
  ClearCacedTiles();
  Super::OnDestroyTileset();
}

void AVrdCesium3DTilesetBase::OnLoadTilesetCompleted()
{
  if (GetIsSaveUrlToUassetInProgress() /*&& !GetCachedTiles().IsEmpty()*/)
  {
      SaveCachedTilesToUasset();
      SaveUrlToUassetState = ESaveUrlToUassetState::Completed;
  }
}

void AVrdCesium3DTilesetBase::BeginPlay()
{
	Super::BeginPlay();

  SaveUrlToUassetState          = ESaveUrlToUassetState::None; 
}

void AVrdCesium3DTilesetBase::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);
  
  if (!FMath::IsNearlyEqual(this->GetLoadProgress(), 100.0)) 
  {
    UE_LOG(LogTemp, Warning, TEXT("getNumberOfTilesLoaded: %d"), this->GetTileset()->getNumberOfTilesLoaded());
  }

  if (isTestSaveUrlToUasset) 
  {
    SaveUrlToUasset(SaveUrl, SaveUrlDir);
    isTestSaveUrlToUasset = false;
  }

  /*if (GetIsSaveUrlToUassetCompleted())
  {
    ResetSaveUrlToUassetState();
  }*/
}

void AVrdCesium3DTilesetBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  Super::EndPlay(EndPlayReason);

  if (EndPlayReason == EEndPlayReason::Type::Quit)
  {
    UE_LOG(LogTemp, Warning, TEXT("EEndPlayReason::Type::Quit"));
  }

  if (EndPlayReason == EEndPlayReason::Type::EndPlayInEditor)
  {
    //OnConstruction({});
    //LoadTileset();
  }
}

bool AVrdCesium3DTilesetBase::ShouldTickIfViewportsOnly() const
{
  return bIsTickInEditor;
}

void AVrdCesium3DTilesetBase::OnConstruction(const FTransform& transform) {
    Super::OnConstruction(transform);
}

void AVrdCesium3DTilesetBase::ResetSaveUrlToUassetState()
{
  SaveUrlToUassetState = ESaveUrlToUassetState::None;
  PrimaryActorTick.bCanEverTick = true;
  DestroyTileset();
}

bool AVrdCesium3DTilesetBase::SaveUrlToUasset(const FString& InUrl, const FString& InSaveDir)
{
    PrimaryActorTick.bCanEverTick = false;
    bIsTickInEditor = false;

    SaveUrlToUassetState = ESaveUrlToUassetState::InProgress;
    SaveUrlDir = InSaveDir;

    DestroyTileset();
    SetUrl(InUrl);
    LoadTileset();

    return true;
}

bool AVrdCesium3DTilesetBase::DeleteUrlUasset(const FString& InUrl, const FString& InDir)
{
    DestroyTileset();
    SetUrl(InUrl);
    LoadTileset();

    return true;
}

bool AVrdCesium3DTilesetBase::GetIsSaveUrlToUassetReady() const {
  return SaveUrlToUassetState == ESaveUrlToUassetState::None || SaveUrlToUassetState == ESaveUrlToUassetState::Completed;
}

bool AVrdCesium3DTilesetBase::GetIsSaveUrlToUassetInProgress() const {
  return SaveUrlToUassetState == ESaveUrlToUassetState::InProgress;
}

bool AVrdCesium3DTilesetBase::GetIsSaveUrlToUassetCompleted() const {
  bool IsCompleted = SaveUrlToUassetState == ESaveUrlToUassetState::Completed;
  /*if (IsCompleted) {
    IsLoadCompleted = false;
  }*/
  return IsCompleted;
}

void AVrdCesium3DTilesetBase::SaveCachedTilesToUasset()
{
    //check(tileMeshes);
    FMeshBuildSettings BuildSettings = {};
    BuildSettings.bComputeWeightedNormals       = false;
    BuildSettings.bRecomputeNormals             = false;
    BuildSettings.bRecomputeTangents            = false;
    BuildSettings.bGenerateLightmapUVs          = false;
    BuildSettings.bBuildReversedIndexBuffer     = false;
    BuildSettings.bBuildReversedIndexBuffer     = false;
    BuildSettings.bGenerateLightmapUVs          = false;

    FMeshNaniteSettings NaniteSettings = {};
    NaniteSettings.bEnabled = true;
    //NaniteSettings.bEnabled = isEnableNaniteWhenLoad;

    FSavePackageArgs SaveArgs = {};
    SaveArgs.TopLevelFlags  = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags      = SAVE_NoError | SAVE_Async;
    
    auto& Tiles = GetCachedTiles();

    TMap<FName, FTileMesh> tileMeshes;
    tileMeshes.Reserve(Tiles.Num());

    const FString OutputDir = GetExportDirectory();

    for (size_t i = 0; i < Tiles.Num(); i++)
    {
        auto& Tile = Tiles[i];
        SaveTileToUasset(Tile, OutputDir, BuildSettings, NaniteSettings, SaveArgs);
        UE_LOG(LogTemp, Warning, TEXT("finished saving %d/%d"), i + 1, Tiles.Num());

        #if UE_BUILD_DEVELOPMENT
        auto name = FName{Tile->name};
        check(tileMeshes.Find(name) == nullptr);
        tileMeshes.Add({name, {}});
        #endif // UE_BUILD_DEVELOPMENT
    }

    UPackage::WaitForAsyncFileWrites();
}

UCachedTile* AVrdCesium3DTilesetBase::CacheTileStaticMesh(const CesiumGltf::Model& Model, const FString& Name, TUniquePtr<class FStaticMeshRenderData>&& RenderData)
{
  auto* vrdTile = NewObject<UCachedTile>();
  vrdTile->name = GetMeshName(Name);
  vrdTile->meshComp = NewObject<UStaticMeshComponent>();

  auto* pStaticMesh = NewObject<UStaticMesh>(vrdTile->meshComp);
  pStaticMesh->NeverStream = true;
  vrdTile->meshComp->SetStaticMesh(pStaticMesh);
  pStaticMesh->SetRenderData(std::move(RenderData));

  return vrdTile; 
}

bool AVrdCesium3DTilesetBase::IsCachedTileUassetExist(const CesiumGltf::Model& Model, const FString& LoadName) const
{
    FString Path = GetCachedTileUassetAbspath(Model, LoadName);
    return FPaths::FileExists(Path);
}

FString AVrdCesium3DTilesetBase::GetMeshUri(const CesiumGltf::Model& Model)
{
    FString Uri;
  
    auto urlIt = Model.extras.find("Cesium3DTiles_TileUrl");
    if (urlIt != Model.extras.end())
    {
        Uri = UTF8_TO_TCHAR(urlIt->second.getStringOrDefault("glTF").c_str());
        // name = constrainLength(name, 256);
    }

    int32 index = 0;
    bool hasFound = Uri.FindLastChar('/', index);
    if (hasFound)
    {
        Uri.RemoveAt(0, index + 1);
    }
    
    return FPaths::GetBaseFilename(Uri);
}

FString AVrdCesium3DTilesetBase::GetUrlFilename(const FString& InUrl)
{
  FString Out = InUrl;
  FString IgnoreChars = TEXT(":/.-");
  for (size_t i = 0; i < Out.Len(); i++) {
    auto ch = Out[i];

    #if 0
    for (auto& Val : IgnoreChars) {
      if (ch == Val) {
        //Out[i] = ch == ':' || ch == '/' || ch == '-' || ch == '.' ? '-' : Out[i];
        Out[i] = TEXT('-');
        break;
      }
    }
    #endif // 0

    Out[i] = ch == ':' || ch == '/' || ch == '-' || ch == '.' ? '-' : Out[i];
  }
  return Out;
}

FString AVrdCesium3DTilesetBase::GetCachedTileUassetAbspath(const CesiumGltf::Model& Model, const FString& LoadName) const
{
  FString Dir = GetExportDirectory().Replace(TEXT("/Game/"), TEXT("/Content/"));
  FString Uri = GetMeshName(LoadName); //GetMeshUri(Model);
  Uri.Append(FPackageName::GetAssetPackageExtension());
  FString Path = FPaths::Combine(FPaths::ProjectDir(), Dir, Uri);
  return Path;
}

FString AVrdCesium3DTilesetBase::GetCachedTileUassetGamepath(const CesiumGltf::Model& Model, const FString& LoadName) const
{
  FString Dir = GetExportDirectory().Replace(TEXT("/Content/"), TEXT("/Game/"));
  FString Uri = GetMeshName(LoadName); //GetMeshUri(Model);
  FString GameName = FString::Printf(TEXT("%s.%s"), *Uri, *Uri);
  FString Path = FPaths::Combine(Dir, GameName);
  return Path;
}

FString AVrdCesium3DTilesetBase::GetMeshName(const FString& Name) const
{
    /*int32 Index;
    Name.FindLastChar('/', Index);
    if (Index != INDEX_NONE)
    {
        return Name.RightChop(Index + 1);
    }
    return {};*/
    FString Ret = FPaths::GetCleanFilename(Name);
    Ret.ReplaceCharInline('.', '_');
    Ret.ReplaceCharInline(' ', '-');
    return Ret;
    //return FPaths::GetBaseFilename(Name);
}

void AVrdCesium3DTilesetBase::SaveTileToUasset(UCachedTile* Tile, const FString& Outdir, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings, FSavePackageArgs SaveArgs)
{
    if (!Tile || !Tile->meshComp)
    {
        return;
    }

    const FString& Uri = Tile->name;
    UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>();
    NewStaticMesh->SetFlags(RF_Public | RF_Standalone);
    
    auto StaticMesh = Tile->meshComp->GetStaticMesh();
    MeshUtil::StaticMesh_CopyFromRenderDataTo(NewStaticMesh, StaticMesh->GetRenderData(), BuildSettings, NaniteSettings);
    AssetUtil::SaveUObject(NewStaticMesh, Uri, Outdir, SaveArgs);
}

void AVrdCesium3DTilesetBase::ReserveCachedTiles(int32 Num) {
  _cachedTiles.Reserve(Num);
}

void AVrdCesium3DTilesetBase::AddCachedTile(UCachedTile* Value) {

    if (!Value)
    {
      return;
    }

#if UE_BUILD_DEVELOPMENT
    {
      for (auto& e : _cachedTiles) 
      {
        if (FName {e->name} == FName {Value->name}) 
        {
            UE_LOG(LogTemp, Warning, TEXT("not a unique name for mesh: %s"), *Value->name);
        }
      }
    }
#endif

    _cachedTiles.Add(Value);
}

void AVrdCesium3DTilesetBase::SetCachedTiles(TArray<UCachedTile*>&& Value) 
{
  _cachedTiles = std::move(Value);
}

void AVrdCesium3DTilesetBase::CreateTileMeshes(const FString& AssetName, const FString& OutputDir)
{
  if (!tileMeshes) 
  {
    //FString OutputDir = FString::Printf(TEXT("%s/%s"), TEXT("/Game/cesium/export_test"), *this->GetName());
    auto DataName = FString::Printf(TEXT("%s_TileTransform"), *AssetName);
    auto* Data = NewObject<UTileMeshes>(GetTransientPackage(), FName{DataName}, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
    AssetUtil::SaveUObject(Data, DataName, OutputDir);
    tileMeshes = Data;
  }
}

void AVrdCesium3DTilesetBase::ClearCacedTiles()
{
  if (isLoadFromPak) 
  {
    for (auto& e : _cachedTiles) 
    {
      if (!e)
      {
        continue;
      }

      if (e->meshComp)
      {
          e->meshComp->SetStaticMesh(nullptr);
      }
    }
  }
  _cachedTiles.Empty();
}

FString AVrdCesium3DTilesetBase::GetExportDirectory() const
{
  return FPaths::Combine(SaveUrlDir, GetUrlFilename(GetUrl()));
}

AVrdCesiumSaveUrlsToUassetMonitor::AVrdCesiumSaveUrlsToUassetMonitor()
  : Super()
{
	PrimaryActorTick.bCanEverTick = true;
  isTestSaveUrlsToUasset = false;
  // this->RootComponent = CreateDefaultSubobject<USceneComponent>(USceneComponent::StaticClass()->GetFName());
}

void AVrdCesiumSaveUrlsToUassetMonitor::RequestSaveUrlsToUasset(const TArray<FString>& InUrls, const FString& InSaveDir, bool IsCloseUnrealAfterCompleted_)
{
  if (InUrls.IsEmpty())
  {
    IsSaveUrlsToUassetInProgress = false;
    return;
  }

  if (Tileset)
  {
    Tileset->ResetSaveUrlToUassetState();
    IsSaveUrlsToUassetInProgress = true;
    IsCloseUnrealAfterCompleted = IsCloseUnrealAfterCompleted_;
    Urls = InUrls;
    curUrlIndex = 0;
  }

  TilesetLoader.Reset();
}

void AVrdCesiumSaveUrlsToUassetMonitor::CancelSaveUrlsToUasset()
{
  if (Tileset)
  {
    Tileset->ResetSaveUrlToUassetState();
  }
  TilesetLoader.Reset();
  isCancelSaveUrlsToUasset = false;
}

void AVrdCesiumSaveUrlsToUassetMonitor::BeginPlay()
{
  Super::BeginPlay();

  isTestSaveUrlsToUasset = false;
}

void AVrdCesiumSaveUrlsToUassetMonitor::Tick(float DeltaTime)
{
  Super::Tick(DeltaTime);

  if (isTestSaveUrlsToUasset)
  {
    RequestSaveUrlsToUasset(Urls, SaveDir, IsCloseUnrealAfterCompleted);
    isTestSaveUrlsToUasset = false;
  }

  if (isCancelSaveUrlsToUasset)
  {
    CancelSaveUrlsToUasset();
  }

  if (IsSaveUrlsToUassetInProgress)
  {
    SaveUrlsToUassetUpdate(DeltaTime);
  }
}

bool AVrdCesiumSaveUrlsToUassetMonitor::ShouldTickIfViewportsOnly() const
{
  return true;
}

void AVrdCesiumSaveUrlsToUassetMonitor::SaveUrlsToUassetUpdate(float DeltaTime)
{
  if (!Tileset)
  {
    return;
  }

  bool IsCompleted = curUrlIndex >= Urls.Num();
  if (IsCompleted) {
    IsSaveUrlsToUassetInProgress = false;
    if (IsCloseUnrealAfterCompleted)
    {
      if (GEngine && GEngine->AssetManager/* && !GEngine->AssetManager->GetAssetRegistry().IsLoadingAssets()*/)
      {
        GEngine->AssetManager->GetAssetRegistry().WaitForCompletion();
        FPlatformMisc::RequestExit(false);
      }
    }
    return;
  }

  if (Tileset->GetIsSaveUrlToUassetCompleted() || TilesetLoader.IsCompleted())
  {
    UE_LOG(LogTemp, Warning, TEXT("*** cesium load url (%d/%d) completed, url : [%s]"), curUrlIndex + 1, Urls.Num(), *Urls[curUrlIndex]);
    curUrlIndex++;
    TilesetLoader.Reset();
    Tileset->ResetSaveUrlToUassetState();
  }
  else if (Tileset->GetIsSaveUrlToUassetInProgress())
  {

  }
  else if (Tileset->GetIsSaveUrlToUassetReady())
  {
    Tileset->SaveUrlToUasset(Urls[curUrlIndex], SaveDir);
    TilesetLoader.RequestLoadTileset(Tileset, Urls[curUrlIndex], LoadBatchCount, TilesetTimeout);
  }

  TilesetLoader.Tick(DeltaTime);
}

void FVrdTilesetLoader::RequestLoadTileset(AVrdCesium3DTilesetBase* Tileset, FString Url_, uint32 BatchCount_, float TimesetTimeout_)
{
  Reset();
  VrdTileset = Tileset;
  BatchCount = BatchCount_;
  LoadingState = EVrdTilesetLoadingState::None;
  TilesetTimeout = TimesetTimeout_;
  // FGenericPlatformProcess::ConditionalSleep(fnIsSaveCompleted, 0.1f);*/
}

void FVrdTilesetLoader::CancelLoadTileset()
{
  if (VrdTileset)
  {
    VrdTileset->ResetSaveUrlToUassetState();
  }
  Reset();
}

void FVrdTilesetLoader::Tick(float DeltaTime)
{
  if (!VrdTileset)
  {
    return;
  }

  auto* rootTileset = VrdTileset->GetTileset();
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

void FVrdTilesetLoader::Reset()
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

void FVrdTilesetLoader::_getAllTiles(Tile* Tile_, Tileset* Tileset_)
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

void FVrdTilesetLoader::_loadTileBatchBegin(Tile* RootTile_, Tileset* Tileset_)
{
  LoadTileListBatch.addRange(TileList.getTiles(), LastLoadedTileCount, BatchCount);
  LoadingState = EVrdTilesetLoadingState::LoadBatch;
}

void FVrdTilesetLoader::_loadTileBatchTick(Tile* RootTile_, Tileset* Tileset_)
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

void FVrdTilesetLoader::_loadTileBatchEnd(Tile* RootTile_, Tileset* Tileset_)
{
  VrdTileset->SaveCachedTilesToUasset();
  VrdTileset->ClearCacedTiles();

  // if unload parent, if may affect the flatten tileList
  unLoadTileListIfLeaf(LoadTileListBatch);
  unLoadTileListIfLeaf(LoadTileListSubBatch);

  if (LastLoadedTileCount >= TileList.Num())
  {
    LoadingState = EVrdTilesetLoadingState::Completed;
  } else
  {
    LoadingState = EVrdTilesetLoadingState::LoadBatchBegin;
  }
}

void FVrdTilesetLoader::_loadTilesetCompleted(Tile* RootTile_, Tileset* Tileset_)
{

}

void FVrdTilesetLoader::unLoadTileListIfLeaf(FTileList& TileList_)
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

int32 FVrdTilesetLoader::GetTotalBatchCount() const { return FMath::CeilToInt(TileList.Num() / (float)BatchCount); }

bool FVrdTilesetLoader::IsCompleted() const { return LoadingState == EVrdTilesetLoadingState::Completed || LoadingState == EVrdTilesetLoadingState::Failed; }
