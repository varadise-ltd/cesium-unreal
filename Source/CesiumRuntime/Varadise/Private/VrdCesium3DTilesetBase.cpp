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
  ClearCachedTiles();
  CachedMeshLoader.Reset();
  TilesetLoader.Reset();

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

void AVrdCesium3DTilesetBase::ClearCachedTiles()
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
