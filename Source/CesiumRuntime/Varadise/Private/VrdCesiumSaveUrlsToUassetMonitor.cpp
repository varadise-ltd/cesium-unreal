#include "VrdCesiumSaveUrlsToUassetMonitor.h"

#include "Components/StaticMeshComponent.h"
#include "MaterialDomain.h"

#include "CesiumRuntime/Private/CesiumGltfComponent.h"
#include "CesiumRuntime/Private/VecMath.h"
#include "CesiumRuntime/Private/CesiumPrimitive.h"

#include <DistanceFieldAtlas.h>
#include <MeshCardBuild.h>

#include "FileHelpers.h"

AVrdCesiumSaveUrlsToUassetMonitor::AVrdCesiumSaveUrlsToUassetMonitor()
  : Super()
{
	PrimaryActorTick.bCanEverTick = true;
  isTestSaveUrlsToUasset = false;
  // this->RootComponent = CreateDefaultSubobject<USceneComponent>(USceneComponent::StaticClass()->GetFName());
}

void AVrdCesiumSaveUrlsToUassetMonitor::RequestSaveUrlsToUasset(const TArray<FString>& InUrls, const FString& InSaveDir, bool bIsWaitMeshPostprocessing_, bool IsCloseUnrealAfterCompleted_)
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
    bIsWaitMeshPostprocessing = bIsWaitMeshPostprocessing_;
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
    RequestSaveUrlsToUasset(Urls, SaveDir, bIsWaitMeshPostprocessing, IsCloseUnrealAfterCompleted);
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

void AVrdCesiumSaveUrlsToUassetMonitor::SaveAllPackages(bool bIsDitryOnly)
{
  if (GEditor)
    {
        // Save all dirty packages and levels
        bool bPromptUser = false; // Set to false to avoid UI prompts
        bool bSaveMapPackages = true; // Save level packages
        bool bSaveContentPackages = true; // Save asset packages
        bool bCheckDirty = bIsDitryOnly; // Only save if dirty
        bool bFastSave = false; // Full save, not just quick metadata

        FEditorFileUtils::SaveDirtyPackages(
            bPromptUser,
            bSaveMapPackages,
            bSaveContentPackages,
            bCheckDirty,
            bFastSave
        );

        UE_LOG(LogTemp, Log, TEXT("All dirty packages have been saved."));
    }
}

void AVrdCesiumSaveUrlsToUassetMonitor::SaveUrlsToUassetUpdate(float DeltaTime)
{
  bool IsCompleted = curUrlIndex >= Urls.Num();
  if (IsCompleted) {
    if (bIsWaitMeshPostprocessing)
    {
      bool bIsMeshPostprocessingCompleted = true;
      if (GDistanceFieldAsyncQueue)
      {
        bIsMeshPostprocessingCompleted &= GDistanceFieldAsyncQueue->GetNumOutstandingTasks() == 0;
        //GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete();
      }

      if (GCardRepresentationAsyncQueue)
      {
        bIsMeshPostprocessingCompleted &= GCardRepresentationAsyncQueue->GetNumOutstandingTasks() == 0;
        //GCardRepresentationAsyncQueue->BlockUntilAllBuildsComplete();
      }

      if (!bIsMeshPostprocessingCompleted)
      {
        return;
      }
    }

    SaveAllPackages();
    if (IsCloseUnrealAfterCompleted)
    {
      if (GEngine && GEngine->AssetManager/* && !GEngine->AssetManager->GetAssetRegistry().IsLoadingAssets()*/)
      {
        GEngine->AssetManager->GetAssetRegistry().WaitForCompletion();
        FPlatformMisc::RequestExit(false);
      }
    }

    IsSaveUrlsToUassetInProgress = false;
    return;
  }

  if (!Tileset)
  {
    return;
  }


  if (Tileset->GetIsSaveUrlToUassetCompleted() || TilesetLoader.IsCompleted())
  {
    UE_LOG(LogTemp, Warning, TEXT("*** cesium load url (%d/%d) completed, url : [%s]"), curUrlIndex + 1, Urls.Num(), *Urls[curUrlIndex]);
    curUrlIndex++;
    TilesetLoader.Reset();
    Tileset->ResetSaveUrlToUassetState();
    SaveAllPackages();
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
