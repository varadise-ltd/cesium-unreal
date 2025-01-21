#include "CesiumRuntime/Varadise/Public/Common/VrdCesium_Common.h"

void MeshUtil::GetRawMeshTo(FRawMesh& Out, const FStaticMeshVertexBuffers& VtxBufs, const FRawStaticIndexBuffer& IdxBuf)
{
    {
        auto& buf = VtxBufs.PositionVertexBuffer;
        Out.VertexPositions.SetNum(buf.GetNumVertices());
        memcpy(Out.VertexPositions.GetData(), VtxBufs.PositionVertexBuffer.GetVertexData(), VtxBufs.PositionVertexBuffer.GetAllocatedSize());
    }

    const auto      idxCount = IdxBuf.GetNumIndices();
    const auto&     indices  = Out.WedgeIndices;

    {
        auto& dst = Out.WedgeIndices;
        dst.SetNum(idxCount);
        for (size_t i = 0; i < dst.Num(); i++) 
        {
            dst[i] = IdxBuf.GetIndex(i);
        }
    }

    {
        int32 NumFaces  = idxCount / 3;
        Out.FaceMaterialIndices.SetNum(NumFaces);
        Out.FaceSmoothingMasks.SetNum(NumFaces);
    }

    if (auto NumTexCoords = VtxBufs.StaticMeshVertexBuffer.GetNumTexCoords())
    {
        for (size_t j = 0; j < NumTexCoords; j++) 
        {
            auto& dst = Out.WedgeTexCoords[j];
            dst.SetNum(idxCount);
            for (size_t i = 0; i < dst.Num(); i++) 
            {
                dst[i] = VtxBufs.StaticMeshVertexBuffer.GetVertexUV(indices[i], j);
            }
        }
    }

    {
        auto& dst = Out.WedgeTangentX;
        dst.SetNum(idxCount);
        for (size_t i = 0; i < dst.Num(); i++) 
        {
            dst[i] = VtxBufs.StaticMeshVertexBuffer.VertexTangentX(indices[i]);
        }
    }

    {
        auto& dst = Out.WedgeTangentY;
        dst.SetNum(idxCount);
        for (size_t i = 0; i < dst.Num(); i++) 
        {
            dst[i] = VtxBufs.StaticMeshVertexBuffer.VertexTangentY(indices[i]);
        }
    }

    {
        auto& dst = Out.WedgeTangentZ;
        dst.SetNum(idxCount);
        for (size_t i = 0; i < dst.Num(); i++) 
        {
            dst[i] = VtxBufs.StaticMeshVertexBuffer.VertexTangentZ(indices[i]);
        }
    }

    {
        auto& dst = Out.WedgeColors;
        VtxBufs.ColorVertexBuffer.GetVertexColors(dst);
    }
}

#if WITH_EDITOR

void MeshUtil::StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, FStaticMeshRenderData* Src, const TArray<FStaticMaterial>& StaticMaterials, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings)
{
    check(Out);

    auto* RenderData = Src;
    if (!RenderData)
    {
        return;
    }

    Out->PreEditChange(nullptr);

    auto& LodRscs = RenderData->LODResources;
    for (size_t i = 0; i < LodRscs.Num(); i++) 
    {
        auto* Rsc = &LodRscs[i];

        FRawMesh RawMesh;
        MeshUtil::GetRawMeshTo(RawMesh, Rsc->VertexBuffers, Rsc->IndexBuffer);
        auto& SrcModel = Out->AddSourceModel();
        SrcModel.BuildSettings = BuildSettings;
        SrcModel.SaveRawMesh(RawMesh);
    }

    Out->NaniteSettings = NaniteSettings;
    Out->SetStaticMaterials(StaticMaterials);
    Out->Build(false);
    Out->PostEditChange();
}

void MeshUtil::StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, FStaticMeshRenderData* Src, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings)
{
    TArray<FStaticMaterial> Materials;
    StaticMesh_CopyFromRenderDataTo(Out, Src, Materials, BuildSettings, NaniteSettings);
}

void MeshUtil::StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, UStaticMesh* Src, const TArray<FStaticMaterial>& StaticMaterials, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings)
{
    if (!Src)
    {
        return;
    }
    auto* RenderData = Src->GetRenderData();
    StaticMesh_CopyFromRenderDataTo(Out, RenderData, StaticMaterials, BuildSettings, NaniteSettings);
}

UStaticMesh* MeshUtil::DuplicateStaticMesh(UStaticMesh* SourceMesh, const FString& Name, UObject* Owner, EObjectFlags ObjectFlags)
{
    if (!SourceMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("Source mesh is null"));
        return nullptr;
    }

    // Create a new static mesh
    UStaticMesh* NewMesh = NewObject<UStaticMesh>(Owner, FName{Name}, ObjectFlags);

    // Copy static mesh properties
    NewMesh->PreEditChange(nullptr);
    NewMesh->SetStaticMaterials(SourceMesh->GetStaticMaterials());
    NewMesh->SetLightingGuid();

    // Copy LOD info
    for (int32 LODIndex = 0; LODIndex < SourceMesh->GetNumSourceModels(); ++LODIndex)
    {
        NewMesh->AddSourceModel();
        FStaticMeshSourceModel& SourceModel = SourceMesh->GetSourceModel(LODIndex);
        FStaticMeshSourceModel& DestModel = NewMesh->GetSourceModel(LODIndex);

        DestModel.BuildSettings = SourceModel.BuildSettings;
        DestModel.ReductionSettings = SourceModel.ReductionSettings;

        // Copy the mesh description
        FMeshDescription* SourceMeshDescription = SourceMesh->GetMeshDescription(LODIndex);
        if (SourceMeshDescription)
        {
            FMeshDescription* NewMeshDescription = NewMesh->CreateMeshDescription(LODIndex);
            *NewMeshDescription = *SourceMeshDescription;
            NewMesh->CommitMeshDescription(LODIndex);
        }
        //NewMesh->SetRenderData(SourceMesh->GetRenderData());
    }

    // Register the new static mesh with the asset registry
    FAssetRegistryModule::AssetCreated(NewMesh);
    NewMesh->PostEditChange();

    return NewMesh;
}

#else

void MeshUtil::StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, FStaticMeshRenderData* Src, const TArray<FStaticMaterial>& StaticMaterials, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings) 
{

}

void MeshUtil::StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, FStaticMeshRenderData* Src, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings) 
{

}

void MeshUtil::StaticMesh_CopyFromRenderDataTo(UStaticMesh* Out, UStaticMesh* Src, const TArray<FStaticMaterial>& StaticMaterials, const FMeshBuildSettings& BuildSettings, const FMeshNaniteSettings& NaniteSettings) 
{

}

UStaticMesh* MeshUtil::DuplicateStaticMesh(UStaticMesh* SourceMesh, const FString& Name, UObject* Owner, EObjectFlags ObjectFlags)
{
    return nullptr;
}


#endif

UPackage* AssetUtil::SaveUObject(UObject* UObj, const FString& AssetName, const FString& PackagePath, FSavePackageArgs SaveArgs)
{
    if (!UObj)
    {
        return nullptr;
    }

    FString PackageName = FPaths::Combine(*PackagePath, *AssetName);

    UPackage* Package = CreatePackage(*PackageName);
    check(Package);

    UObj->Rename(*AssetName, Package);

    /*
     * no .uasset extension will only save on virtual disk
    */
    FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    if (UPackage::SavePackage(Package, UObj, *PackageFilename, SaveArgs))
    {
        UE_LOG(LogTemp, Log, TEXT("Static mesh '%s' created and saved successfully"), *PackageFilename);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save static mesh '%s'"), *PackageFilename);
    }
    
    FAssetRegistryModule::AssetCreated(UObj);
    UObj->MarkPackageDirty();

    return Package;
}

UPackage* AssetUtil::SaveUObject(UObject* UObj, const FString& AssetName, const FString& PackagePath)
{
    FSavePackageArgs SaveArgs = {};
    SaveArgs.TopLevelFlags  = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags      = SAVE_NoError;
    return SaveUObject(UObj, AssetName, PackagePath, SaveArgs);
}

void UTileMeshes::SpwanAllToWorld(UWorld* World)
{
    auto* Actor = World->SpawnActor(AActor::StaticClass());
    if (!Actor)
		return;

    {
      auto* NewComponent = NewObject<UBillboardComponent>(Actor);
      NewComponent->RegisterComponent();
      NewComponent->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
      Actor->AddInstanceComponent(NewComponent);
      Actor->SetRootComponent(NewComponent);
    }

    check(World);
	for (auto& e : tileMeshes) 
	{
		auto* NewComponent = NewObject<UStaticMeshComponent>(Actor);
		NewComponent->RegisterComponent();
		NewComponent->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Actor->AddInstanceComponent(NewComponent);

		NewComponent->SetStaticMesh(e.Value.mesh);

		//Actor->SetActorTransform(e.Value.transform);
    NewComponent->SetWorldTransform(e.Value.transform);
	}
}
