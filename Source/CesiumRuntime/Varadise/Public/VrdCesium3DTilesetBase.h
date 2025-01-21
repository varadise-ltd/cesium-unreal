// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/VrdCesium_Common.h"
#include "CesiumRuntime/Public/Cesium3DTileset.h"
#include "CesiumRuntime/Public/CesiumPrimitiveFeatures.h"

#include "VrdCesium3DTilesetBase.generated.h"

UENUM()
enum class ESaveUrlToUassetState : uint8 { None, InProgress, Completed, _Count };

UCLASS()
class CESIUMRUNTIME_API AVrdCesium3DTilesetBase : public ACesium3DTileset 
{
	GENERATED_BODY()
public:
	using CsmTile = Cesium3DTilesSelection::Tile;

public:
  UPROPERTY(EditAnywhere) bool isLoadFromPak = false;		      // temporary
  UPROPERTY(EditAnywhere) UTileMeshes* tileMeshes = nullptr;  // temporary

  UPROPERTY(EditAnywhere, Transient) bool isTestSaveUrlToUasset = false;     // temporary
  UPROPERTY(EditAnywhere) FString SaveUrl = "";                              // temporary

  UPROPERTY(EditAnywhere) FString SaveUrlDir = "";
  UPROPERTY(EditAnywhere, Transient) uint8   IsLoadFromUasset               : 1;

  UPROPERTY(EditAnywhere, Transient) ESaveUrlToUassetState SaveUrlToUassetState = ESaveUrlToUassetState::None;
  //UPROPERTY(EditAnywhere, Transient) uint8   IsSaveUrlToUassetInProgress    : 1;
  //UPROPERTY(EditAnywhere, Transient) uint8   IsSaveUrlToUassetCompleted     : 1;

public:
	AVrdCesium3DTilesetBase();
  virtual ~AVrdCesium3DTilesetBase();

public:
  UFUNCTION(BlueprintCallable) void ResetSaveUrlToUassetState();
  UFUNCTION(BlueprintCallable) bool SaveUrlToUasset(const FString& InUrl, const FString& InSaveDir);
  UFUNCTION(BlueprintCallable) bool DeleteUrlUasset(const FString& InUrl, const FString& InDir);

  UFUNCTION(BlueprintCallable) bool GetIsSaveUrlToUassetReady() const;
  UFUNCTION(BlueprintCallable) bool GetIsSaveUrlToUassetInProgress() const;
  UFUNCTION(BlueprintCallable) bool GetIsSaveUrlToUassetCompleted() const;

  UFUNCTION() void SaveCachedTilesToUasset();

  UCachedTile* CacheTileStaticMesh(const CesiumGltf::Model& Model, const FString& Name, TUniquePtr<class FStaticMeshRenderData>&& RenderData);
  //UStaticMesh* SaveTileStaticMesh(const CesiumGltf::Model& Model, FStaticMeshRenderData* RenderData);
  void SaveTileToUasset(UCachedTile* Tile, const FString& Outdir, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings, FSavePackageArgs SaveArgs);

public:
  static FString GetMeshUri(const CesiumGltf::Model& Model);
  static FString GetUrlFilename(const FString& InUrl);

  bool IsCachedTileUassetExist(const CesiumGltf::Model& Model, const FString& Name) const;
  FString GetMeshName(const FString& Name) const;

public:
  void ReserveCachedTiles(int32 Num);
  void AddCachedTile(UCachedTile* Value);
  void SetCachedTiles(TArray<UCachedTile*>&& Value);

  void CreateTileMeshes(const FString& AssetName, const FString& OutputDir);

public:
  TArray<UCachedTile*>& GetCachedTiles();
  FString GetExportDirectory() const;

protected:
  virtual void OnDestroyTileset() override;
  virtual void OnLoadTilesetCompleted() override;

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void OnConstruction(const FTransform& transform) override;

protected:	
	UPROPERTY(VisibleAnywhere) TArray<UCachedTile*> _cachedTiles;
};

inline TArray<UCachedTile*>& AVrdCesium3DTilesetBase::GetCachedTiles()
{
  return _cachedTiles;
}

UCLASS()
class CESIUMRUNTIME_API AVrdCesiumSaveUrlsToUassetMonitor : public AActor
{
  GENERATED_BODY()
public:
  UPROPERTY(EditAnywhere)
  TArray<FString> Urls;

  UPROPERTY(EditAnywhere)
  FString SaveDir;

  UPROPERTY(EditAnywhere, Transient)
  bool isTestSaveUrlsToUasset = false;     // temporary

public:
  AVrdCesiumSaveUrlsToUassetMonitor();

public:
  UFUNCTION(BlueprintCallable)
  void RequestSaveUrlsToUasset(const TArray<FString>& InUrls, const FString& InSaveDir, bool IsCloseUnrealAfterCompleted_);

protected:
  virtual void BeginPlay() override;
  virtual void Tick(float DeltaTime) override;

  virtual bool ShouldTickIfViewportsOnly() const override;

protected:
  void SaveUrlsToUassetUpdate();

private:
  UPROPERTY(EditAnywhere)
  AVrdCesium3DTilesetBase* Tileset = nullptr;

  uint8 IsCloseUnrealAfterCompleted   : 1;
  uint8 IsSaveUrlsToUassetInProgress  : 1;

  int32 curUrlIndex = 0;
};

