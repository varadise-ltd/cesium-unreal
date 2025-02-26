// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Common/VrdCesium_Common.h"
#include "../include/Cesium3DTilesSelection/Tileset.h"

class AVrdCesium3DTilesetBase;

class FCesiumTileList 
{
  using Tile = Cesium3DTilesSelection::Tile;

public:
  FCesiumTileList() = default;

public:
  void clear()
  {
    Tiles.Empty();
    TileSet.Empty();
  }

  void addTile(Tile* tile) {

    Tiles.Add(tile);
    TileSet.Add(tile);

    auto children = tile->getChildren();
    for (auto& child : children) {
      addTile(&child);
    }
  }

  void addTileUnqiue(Tile* tile) {

    if (!TileSet.Find(tile)) {
      Tiles.Add(tile);
      TileSet.Add(tile);
    }

    auto children = tile->getChildren();
    for (auto& child : children) {
      addTileUnqiue(&child);
    }
  }
  
  void addTileUnqiueFromSrc(Tile* tile, const FCesiumTileList& TileList) {

    if (!TileList.Find(tile)) {
      Tiles.Add(tile);
      TileSet.Add(tile);
    }

    auto children = tile->getChildren();
    for (auto& child : children) {
      addTileUnqiueFromSrc(&child, TileList);
    }
  }

  void addRange(const TArray<Tile*>& SrcTiles) {
    Tiles.Reserve(SrcTiles.Num());
    TileSet.Reserve(SrcTiles.Num());

    for (auto* tile : SrcTiles) {
      Tiles.Add(tile);
      TileSet.Add(tile);
    }
  }
  
  void addRange(const TArray<Tile*>& SrcTiles, int32 SrcIndex, uint32 Count) {
    
    if (SrcIndex >= SrcTiles.Num()) {
      return;
    }

    Count = FMath::Min(Count, static_cast<uint32>(SrcTiles.Num()) - SrcIndex);

    Tiles.Reserve(Count);
    TileSet.Reserve(Count);
    for (size_t i = SrcIndex; i < SrcIndex + Count; i++) {
      auto* Tile = SrcTiles[i];
      Tiles.Add(Tile);
      TileSet.Add(Tile);
    }
  }

  TArray<Tile*>& getTiles() {
    return this->Tiles;
  }

  int32 Num() const { return this->Tiles.Num(); }
  bool IsEmpty() const { return this->Tiles.IsEmpty(); }

  Tile*const * Find(Tile* tile) const { return TileSet.Find(tile); }

private:
  TArray<Tile*> Tiles;
  TSet<Tile*> TileSet;
};
