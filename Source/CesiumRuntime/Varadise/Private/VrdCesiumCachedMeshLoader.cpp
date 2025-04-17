#include "VrdCesiumCachedMeshLoader.h"

#include "Components/StaticMeshComponent.h"
#include "MaterialDomain.h"

#include "CesiumRuntime/Private/CesiumGltfComponent.h"
#include "CesiumRuntime/Private/VecMath.h"
#include "CesiumRuntime/Private/CesiumPrimitive.h"

UStaticMesh* FVrdCesiumCachedMeshLoader::LoadCachedMesh(const FString& MeshGamepath, TFunction<void()>&& FnPostLoadMesh)
{
  FStreamableHandle* Handle = FindHandle(MeshGamepath);
  if (Handle && Handle->HasLoadCompleted())
  {
    // check(Handle->HasLoadCompleted());
    return Cast<UStaticMesh>(Handle->GetLoadedAsset());
  }

  auto NewHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(FSoftObjectPath{MeshGamepath}, std::move(FnPostLoadMesh));
  AddHandle(MeshGamepath, NewHandle);

  return nullptr;
}

void FVrdCesiumCachedMeshLoader::Reset()
{
  for (auto& Handle : _CachedHandles)
  {
    if (Handle.Value) {
      Handle.Value->CancelHandle();
      Handle.Value->ReleaseHandle();
    }
  }
  //_pendingCompleteHandles.Empty();
  _CachedHandles.Empty();

  #if VRD_CESIUM_DEBUG
  _meshpaths.Empty();
  _loadCachedMeshParams.Empty();
  #endif
}

void FVrdCesiumCachedMeshLoader::Tick(float DeltaTime, UWorld* World)
{

}

void FVrdCesiumCachedMeshLoader::AddHandle(const FString& Name, TSharedPtr<FStreamableHandle>& Handle)
{
  check(_CachedHandles.Find(Name) == nullptr);
  _CachedHandles.Add(Name, Handle);
}

FStreamableHandle* FVrdCesiumCachedMeshLoader::FindHandle(const FString& Name)
{
  auto* it = _CachedHandles.Find(Name);
  return it ? it->Get() : nullptr;
}

UStaticMesh* FVrdCesiumCachedMeshLoader::FindStaticMesh(const FString& Name)
{
  auto* it = FindHandle(Name);
  if (it)
  {
    check(it->HasLoadCompleted());
    //it->WaitUntilComplete();
    auto* LoadedAsset = it->GetLoadedAsset();
    auto* Mesh = Cast<UStaticMesh>(LoadedAsset);
    if (!Mesh)
    {
      UE_LOG(LogTemp, Warning, TEXT("static mesh is nullptr"));
    }
    return Mesh;
  }
  return nullptr;
}

#if VRD_CESIUM_DEBUG

void FVrdCesiumCachedMeshLoader::Debug()
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
}

UStaticMesh* FVrdCesiumCachedMeshLoader::LoadCachedMesh(const FString& MeshGamepath, TFunction<void()>&& FnPostLoadMesh, const FCesiumLoadCachedMesh_TestParams& params)
{
  if (true)
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
    TileMesh.Transform = FTransform(
                        VecMath::createMatrix(PrimData.pTilesetActor->GetCesiumTilesetToUnrealRelativeWorldTransform() * PrimData.HighPrecisionNodeTransform)
                    );
    //TileMeshes->tileMeshes.Add({*MeshGamepath, TileMesh});
    auto NewHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(FSoftObjectPath{MeshGamepath},
      [this, &TileMesh, MeshGamepath]()
      {
        auto* Mesh = this->FindStaticMesh(MeshGamepath);
        TileMesh.Mesh = Mesh;
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

#endif // VRD_CESIUM_DEBUG
