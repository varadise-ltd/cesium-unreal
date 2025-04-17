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
    bIsSaveUrlsToUassetInProgress = false;
    return;
  }

  if (Tileset)
  {
    Tileset->ResetSaveUrlToUassetState();
    bIsSaveUrlsToUassetInProgress = true;
    bIsCloseUnrealAfterCompleted = IsCloseUnrealAfterCompleted_;
    bIsWaitMeshPostprocessing = bIsWaitMeshPostprocessing_;
    Urls = InUrls;
    SaveDir = InSaveDir;
    CurUrlIndex = 0;
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
    RequestSaveUrlsToUasset(Urls, SaveDir, bIsWaitMeshPostprocessing, bIsCloseUnrealAfterCompleted);
    isTestSaveUrlsToUasset = false;
  }

  if (isCancelSaveUrlsToUasset)
  {
    CancelSaveUrlsToUasset();
  }

  if (bIsSaveUrlsToUassetInProgress)
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
  bool IsCompleted = CurUrlIndex >= Urls.Num();
  if (IsCompleted)
  {
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
    if (bIsCloseUnrealAfterCompleted)
    {
      if (GEngine && GEngine->AssetManager/* && !GEngine->AssetManager->GetAssetRegistry().IsLoadingAssets()*/)
      {
        GEngine->AssetManager->GetAssetRegistry().WaitForCompletion();
        FPlatformMisc::RequestExit(false);
      }
    }

    bIsSaveUrlsToUassetInProgress = false;
    return;
  }

  if (!Tileset)
  {
    return;
  }


  if (Tileset->GetIsSaveUrlToUassetCompleted() || TilesetLoader.IsCompleted())
  {
    UE_LOG(LogTemp, Warning, TEXT("*** cesium load url (%d/%d) completed, url : [%s]"), CurUrlIndex + 1, Urls.Num(), *Urls[CurUrlIndex]);
    CurUrlIndex++;
    TilesetLoader.Reset();
    Tileset->ResetSaveUrlToUassetState();
    SaveAllPackages();
  }
  else if (Tileset->GetIsSaveUrlToUassetInProgress())
  {

  }
  else if (Tileset->GetIsSaveUrlToUassetReady())
  {
    Tileset->SaveUrlToUasset(Urls[CurUrlIndex], SaveDir);
    TilesetLoader.RequestLoadTileset(Tileset, Urls[CurUrlIndex], LoadBatchCount, TilesetTimeout, EVrdTilesetLoaderMode::SaveTile);
  }

  TilesetLoader.Tick(DeltaTime);
}
