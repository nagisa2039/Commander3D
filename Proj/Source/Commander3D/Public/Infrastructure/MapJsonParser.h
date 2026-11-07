#pragma once

#include "CoreMinimal.h"

#include "Common/Result.h"
#include "Map/MapLoader.h"

// Why a map JSON text could not be turned into MapData.
enum class EMapJsonError : uint8
{
	None,
	InvalidJson,      // not parseable JSON, or the root is not an object
	InvalidField,     // a required field is missing or has the wrong type / range
	RowShapeMismatch, // "cells" does not have `height` rows of `width` entries
};

// Infrastructure adapter: map JSON text -> format-agnostic MapData.
//
// {
//   "width": 3, "height": 2,
//   "terrains": [ {"moveCost": 1, "passable": true}, ... ],   // index = terrain id
//   "cells": [ [0, 1, 2], [1, 0, 0] ]                          // rows (north to south) of terrain ids
// }
//
// Game-rule validation (positive size, ids inside the table, ...) stays in commander::application::LoadMap.
class COMMANDER3D_API FMapJsonParser
{
public:
	static commander::application::Result<commander::application::MapData, EMapJsonError> Parse(const FString& JsonText);
};
