#include "VrdCesium3DTilesetBase.h"

#include "Components/StaticMeshComponent.h"
#include "MaterialDomain.h"

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

void AVrdCesium3DTilesetBase::testNanite()
{
  if (isClearTestNanite) {
    testNaniteActors.Empty();
    isClearTestNanite = false;
  }

  if (!isTestNanite) {
    return;
  }

  auto* world = GetWorld();
  if (!world) {
    return;
  }

  testNaniteActors.Reserve(testNaniteCount);
  for (size_t i = 0; i < testNaniteCount; i++) {
    auto* actor = world->SpawnActor<AActor>();

    //auto* meshComp = actor->CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    UStaticMeshComponent* meshComp = nullptr;
    {
      auto* newComponent = NewObject<UStaticMeshComponent>(actor);
      newComponent->RegisterComponent();
      newComponent->AttachToComponent(actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
      actor->AddInstanceComponent(newComponent);
      meshComp = newComponent;
    }

    meshComp->SetStaticMesh(testNaniteMesh);
    if (testNaniteMesh->GetStaticMaterials().IsEmpty()) {
      testNaniteMesh->AddMaterial(UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
    }
    UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(testNaniteMaterial, actor);
    meshComp->SetMaterial(0, DynamicMaterial);

    testNaniteActors.Add(actor);
  }

  isTestNanite = false;
}

void AVrdCesium3DTilesetBase::SetupStaticMesh(
    bool bIsInit,
    bool createNavCollision,
    UStaticMesh* pStaticMesh,
    UStaticMeshComponent* pMesh,
  UMaterialInstanceDynamic* pMaterial,
  UCesiumGltfComponent* pGltf,
  TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>& pCollisionMesh)
{
  if (!pStaticMesh)
  {
    return;
  }

  if (bIsInit)
  {
    pStaticMesh->SetBodySetup(nullptr);
    // prevent StaticMesh material slot zero, if yes, MeshComponent cannot
    // override the material...
    // pStaticMesh->GetStaticMaterials().Empty();
    if (pStaticMesh->GetStaticMaterials().IsEmpty())
    {
      pStaticMesh->AddMaterial(UMaterial::GetDefaultMaterial(EMaterialDomain::MD_Surface));
    }

    pStaticMesh->CreateBodySetup();

    if (createNavCollision)
    {
      pStaticMesh->CreateNavCollision(true);
    }
  }

  pMesh->SetStaticMesh(pStaticMesh);
          
  pMaterial->TwoSided = true;
  pMesh->SetMaterial(0, pMaterial);

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::BodySetup)

    UBodySetup* pBodySetup = pMesh->GetBodySetup();

    // pMesh->UpdateCollisionFromStaticMesh();
    pBodySetup->CollisionTraceFlag =
        ECollisionTraceFlag::CTF_UseComplexAsSimple;

    if (pCollisionMesh) {
      #if ENGINE_VERSION_5_4_OR_HIGHER
      pBodySetup->TriMeshGeometries.Add(pCollisionMesh);
      #else
      pBodySetup->ChaosTriMeshes.Add(pCollisionMesh);
      #endif
    }
    // TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
    
    // Mark physics meshes created, no matter if we actually have a collision
    // mesh or not. We don't want the editor creating collision meshes itself in
    // the game thread, because that would be slow.
    pBodySetup->bCreatedPhysicsMeshes = true;
    pBodySetup->bSupportUVsAndFaceRemap =
        UPhysicsSettings::Get()->bSupportUVFromHitResults;
  }

  pMesh->SetMobility(pGltf->Mobility);

  // TODO: uncomment
  //pMesh->SetupAttachment(pGltf);

  {
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::RegisterComponent)
    pMesh->RegisterComponent();
  }
}

void AVrdCesium3DTilesetBase::OnDestroyTileset()
{
  ClearCacedTiles();
  CachedMeshLoader.Reset();
  TilesetLoader.Reset();

  /*if (isLoadFromPak)
  {
    auto& components = GetComponents();
    for (auto* component : components)
    {
      auto* meshComponent = Cast<UStaticMeshComponent>(component);
      if (meshComponent)
      {
        meshComponent->SetStaticMesh(nullptr);
      }
    }
  }*/

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

  TilesetLoader.RequestLoadTileset(this, GetUrl(), LoadBatchCount, TilesetTimeout, EVrdTilesetLoaderMode::Render);
}

void AVrdCesium3DTilesetBase::Tick(float DeltaTime)
{
  if (!bForceRenderAllTile)
  {
    Super::Tick(DeltaTime);
    
    if (!FMath::IsNearlyEqual(this->GetLoadProgress(), 100.0) && this->GetTileset()) 
    {
      UE_LOG(LogTemp, Warning, TEXT("this->GetLoadProgress(): %f, getNumberOfTilesLoaded: %d"), this->GetLoadProgress(), this->GetTileset()->getNumberOfTilesLoaded());
    }
  }
  else
  {
    TilesetLoader.Tick(DeltaTime);
  }
  
  testNanite();

  if (isTestSaveUrlToUasset) 
  {
    SaveUrlToUasset(SaveUrl, SaveUrlDir);
    isTestSaveUrlToUasset = false;
  }

  CachedMeshLoader.Tick(DeltaTime, GetWorld());
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
  return UpdateInEditor;
}

void AVrdCesium3DTilesetBase::OnConstruction(const FTransform& transform)
{
    Super::OnConstruction(transform);
}

void AVrdCesium3DTilesetBase::ResetSaveUrlToUassetState()
{
  DestroyTileset();

  SaveUrlToUassetState = ESaveUrlToUassetState::None;
  UpdateInEditor = true;
  PrimaryActorTick.bCanEverTick = true;
}

bool AVrdCesium3DTilesetBase::SaveUrlToUasset(const FString& InUrl, const FString& InSaveDir)
{
    PrimaryActorTick.bCanEverTick = false;
    UpdateInEditor = false;

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

void AVrdCesium3DTilesetBase::SetForceRenderAllTile(bool bValue)
{
  bForceRenderAllTile = bValue;
  if (bForceRenderAllTile)
  {
    TilesetLoader.RequestLoadTileset(this, GetUrl(), LoadBatchCount, TilesetTimeout, EVrdTilesetLoaderMode::Render);
  }
  else
  {
    TilesetLoader.CancelLoadTileset();
  }
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
    /*{
      for (auto& e : _cachedTiles) 
      {
        if (FName {e->name} == FName {Value->name}) 
        {
            UE_LOG(LogTemp, Warning, TEXT("not a unique name for mesh: %s"), *Value->name);
        }
      }
    }*/
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
    SaveDir = InSaveDir;
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

  if (this->Tileset) {
    LoadCachedMeshCbCounter = Tileset->CachedMeshLoader.LoadCachedMeshCbCounter;
  }

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
    TilesetLoader.RequestLoadTileset(Tileset, Urls[curUrlIndex], LoadBatchCount, TilesetTimeout, EVrdTilesetLoaderMode::SaveTile);
  }

  TilesetLoader.Tick(DeltaTime);
}

void FVrdTilesetLoader::RequestLoadTileset(AVrdCesium3DTilesetBase* Tileset, FString Url_, uint32 BatchCount_, float TimesetTimeout_, LoaderMode Mode_)
{
  Reset();
  VrdTileset = Tileset;
  BatchCount = BatchCount_;
  LoadingState = EVrdTilesetLoadingState::None;
  TilesetTimeout = TimesetTimeout_;

  Mode = Mode_;
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

void FVrdTilesetLoader::LogBatchInfo()
{
  if (!bLogInfo)
  {
    return;
  }

  UE_LOG(LogTemp, Warning, TEXT("FVrdTilesetLoader::GetNumberOfTilesLoaded: %d"), GetNumberOfTilesLoaded());
  UE_LOG(LogTemp, Warning, TEXT("LoadedCount: %d / %d"), LastLoadedTileCount, TileList.Num());
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

void FVrdTilesetLoader::_loadTileBatchEnd(Tile* RootTile_, Tileset* Tileset_)
{
  if (Mode == EVrdTilesetLoaderMode::SaveTile)
  {
    VrdTileset->SaveCachedTilesToUasset();
    VrdTileset->ClearCacedTiles();

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

void FVrdTilesetLoader::_loadTilesetOnCompleted(Tile* RootTile_, Tileset* Tileset_)
{
  LogBatchInfo();
  LoadingState = EVrdTilesetLoadingState::Completed;
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

bool FVrdTilesetLoader::IsCompleted() const {
  return LoadingState == EVrdTilesetLoadingState::Completed ||
         LoadingState == EVrdTilesetLoadingState::Failed;
}

int32 FVrdTilesetLoader::GetNumberOfTilesLoaded() const noexcept
{
  return VrdTileset && VrdTileset->GetTileset() ? VrdTileset->GetTileset()->getNumberOfTilesLoaded() : 0;
}

UStaticMesh* FVrdCachedMeshLoader::LoadCachedMesh(const FString& MeshGamepath, TFunction<void()>&& FnPostLoadMesh, FLoadCachedMesh_TestParams params)
{
  if (false)
  {
    if (!TileMeshes)
    {
      TileMeshes = NewObject<UTileMeshes>(GetTransientPackage(), FName{TileMeshesName}, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
      AssetUtil::SaveUObject(TileMeshes, TileMeshesName, TileMeshesSaveDir);
      TileMeshes->tileMeshes.Reserve(100000); // new reserve will crash, just a workaround
    }

    auto* MeshComp = params.meshComp;
    const auto& PrimData = Cast<ICesiumPrimitive>(MeshComp)->getPrimitiveData();

    FTileMesh& TileMesh = TileMeshes->tileMeshes.Add(*MeshGamepath);
    //TileMesh.mesh = Cast<UStaticMesh>(UAssetManager::GetStreamableManager().LoadSynchronous(FSoftObjectPath{MeshGamepath}));
    TileMesh.transform = FTransform(
                        VecMath::createMatrix(PrimData.pTilesetActor->GetCesiumTilesetToUnrealRelativeWorldTransform() * PrimData.HighPrecisionNodeTransform)
                    );
    //TileMeshes->tileMeshes.Add({*MeshGamepath, TileMesh});
    auto NewHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(FSoftObjectPath{MeshGamepath},
      [this, &TileMesh, MeshGamepath]()
      {
        auto* Mesh = this->FindStaticMesh(MeshGamepath);
        TileMesh.mesh = Mesh;
        SpwanAllToWorldCallbackCounter++;
      });
    AddHandle(MeshGamepath, NewHandle);

    return nullptr;
  }

  check(_meshpaths.Find(MeshGamepath) == nullptr);
  _meshpaths.Add(MeshGamepath);
  _loadCachedMeshParams.Add(params);

  UStaticMesh* pMesh = nullptr;

  FStreamableHandle* Handle = FindHandle(MeshGamepath);
  if (Handle/* && Handle->HasLoadCompleted()*/)
  {
    check(Handle->HasLoadCompleted());
    pMesh = Cast<UStaticMesh>(Handle->GetLoadedAsset());
    return pMesh;
  }

  if (true)
  {
    auto NewHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(FSoftObjectPath{MeshGamepath}, std::move(FnPostLoadMesh));
    AddHandle(MeshGamepath, NewHandle);
  }
  else
  {
    TFunction<void()> fn = []() {};
    auto NewHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(FSoftObjectPath{MeshGamepath}, std::move(fn));
    NewHandle->WaitUntilComplete();
    pMesh = Cast<UStaticMesh>(NewHandle->GetLoadedAsset());
  }


  return pMesh;
}

void FVrdCachedMeshLoader::Reset()
{
  for (auto& Handle : _cachedHandles) {
    if (Handle.Value) {
      Handle.Value->CancelHandle();
    }
  }
  //_pendingCompleteHandles.Empty();
  _cachedHandles.Empty();


  _meshpaths.Empty();
  _loadCachedMeshParams.Empty();
}

void FVrdCachedMeshLoader::Tick(float DeltaTime, UWorld* World)
{

  if (isTestCheckLoadingMesh) {

    if (UAssetManager::GetStreamableManager().AreAllAsyncLoadsComplete()) {
    } else {
      UE_LOG(LogTemp, Warning, TEXT("not all async is completed"));
    }

    for (auto& path : _meshpaths) {
      bool isCompleted =
          UAssetManager::GetStreamableManager().IsAsyncLoadComplete(path);
      if (!isCompleted) {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("loading mesh is not completed: %s"),
            *path.ToString());
      }
    }

    isTestCheckLoadingMesh = false;
  }

  if (isTestSetAllMeshVisible) {
    
    for (auto& e : _loadCachedMeshParams)
    {
      e.meshComp->SetVisibility(true, true);
      e.meshComp->SetVisibleFlag(true);
    }
    isTestSetAllMeshVisible = false;
  }

  if (TileMeshes && isTestSpwanAllToWorld) {
    TileMeshes->SpwanAllToWorld(World);
    isTestSpwanAllToWorld = false;
  }

  if (isTestSpwanMeshToWorld) {
    isTestSpwanMeshToWorld = false;

    TestSpawnMeshHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(FSoftObjectPath{TestSpwanMeshpath},
      [this, World]()
      {
        bool IsActive         = TestSpawnMeshHandle->IsActive();
        bool HasLoadCompleted = TestSpawnMeshHandle->HasLoadCompleted();
        auto* Mesh = Cast<UStaticMesh>(TestSpawnMeshHandle->GetLoadedAsset());

        UTileMeshes::SpwanMeshActor(World, Mesh, {});
      });
  }

  /*TArray<FStreamableHandle*> LoadingHandles;
  LoadingHandles.Reserve(_pendingCompleteHandles.Num());
  for (auto* Handle : _pendingCompleteHandles)
  {
    if (Handle->HasLoadCompleted())
    {
      Handle->WaitUntilComplete();
    }
    else
    {
      LoadingHandles.Add(Handle);
    }
  }
  _pendingCompleteHandles = LoadingHandles;*/

}

void FVrdCachedMeshLoader::AddHandle(const FString& Name, TSharedPtr<FStreamableHandle>& Handle)
{
  check(_cachedHandles.Find(Name) == nullptr);
  _cachedHandles.Add(Name, Handle);
}

FStreamableHandle* FVrdCachedMeshLoader::FindHandle(const FString& Name)
{
  auto* it = _cachedHandles.Find(Name);
  return it ? it->Get() : nullptr;
}

UStaticMesh* FVrdCachedMeshLoader::FindStaticMesh(const FString& Name)
{
  auto* it = FindHandle(Name);
  if (it)
  {
    check(it->HasLoadCompleted());
    //it->WaitUntilComplete();
    auto* LoadedAsset = it->GetLoadedAsset();
    auto* Mesh = Cast<UStaticMesh>(LoadedAsset);
    if (!Mesh) {
      UE_LOG(LogTemp, Warning, TEXT("static mesh is nullptr"));
    }
    return Mesh;
  }
  return nullptr;
}
