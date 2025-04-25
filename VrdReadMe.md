# Cesium Nanite

## How to build
- run build.bat

## Features
- save cesium url as nanite asset
  - batch save each mesh in tileset

## Todo
- full automation cesium nanite pipeline
  - save as pak
    - need test whole pipeline

- handle exception when loading, but no indicator to know it is throwed…

## Working in progress
- full automation cesium nanite pipeline
- TestLoadDLC for testing loading .pak

## Flow
- for save cesium urls
  - use python to kick off unreal (pass a list read in json to unreal), then it will call VrdCesiumSaveUrlsToUassetMonitor::RequestSaveUrlsToUasset()

## Class
- VrdCesium3DTilesetBase
  - override original cesium tile class
  - UCachedTile is for save Tile CesiumGltfComponent.cpp

- VrdCesiumSaveUrlsToUassetMonitor
  - could test in unreal by RequestSaveUrlsToUasset (no need to use python)

- FVrdCesiumTilesetLoader
  - save url with batch (reduce memory usage)

- VrdCesiumTileList
  - useful container for batch save

- VrdCesiumCachedMeshLoader
  - use saved nantie asset for cesium instead of default