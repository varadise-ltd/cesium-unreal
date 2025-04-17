// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/VrdCesium_Common.h"
#include "VrdCesiumTilesetLoader.h"

//#include "Engine/AssetManager.h"

#include "VrdCesiumSaveUrlsToUassetMonitor.generated.h"

struct FStreamableHandle;
class UCesiumGltfComponent;
class AVrdCesium3DTilesetBase;

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
  uint8 bIsWaitMeshPostprocessing : 1;

  UPROPERTY(EditAnywhere, Transient)
  uint8 bIsCloseUnrealAfterCompleted : 1;

  #if 1
  
  UPROPERTY(EditAnywhere, Transient)
  bool isTestSaveUrlsToUasset = false;     // temporary
  
  UPROPERTY(EditAnywhere, Transient)
  bool isCancelSaveUrlsToUasset = false;     // temporary
  
  // TODO: remove
  UPROPERTY(EditAnywhere)
  uint32 LoadCachedMeshCbCounter = 0;

  #endif // 1

public:
  AVrdCesiumSaveUrlsToUassetMonitor();

public:
  UFUNCTION(BlueprintCallable)
  void RequestSaveUrlsToUasset(const TArray<FString>& InUrls, const FString& InSaveDir, bool bIsWaitMeshPostprocessing_, bool IsCloseUnrealAfterCompleted_);

  void CancelSaveUrlsToUasset();

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;

  virtual bool ShouldTickIfViewportsOnly() const override;

  void SaveAllPackages(bool bIsDitryOnly = true);

protected:
  void SaveUrlsToUassetUpdate(float DeltaTime);

private:
  UPROPERTY(EditAnywhere)
  AVrdCesium3DTilesetBase* Tileset = nullptr;

  uint8 bIsSaveUrlsToUassetInProgress  : 1;

  int32 CurUrlIndex = 0;

  FVrdCesiumTilesetLoader TilesetLoader;
};
