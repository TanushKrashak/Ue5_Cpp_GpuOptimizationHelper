#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GpuOptimizationHelper.generated.h"

UENUM(BlueprintType)
enum class ESortKey : uint8 {
	Name UMETA(DisplayName = "Name"),
	MinLOD UMETA(DisplayName = "Min LOD"),
	Vert UMETA(DisplayName = "Vert"),
	Tri UMETA(DisplayName = "Tris"),
	Count UMETA(DisplayName = "Count"),
	TotalVert UMETA(DisplayName = "Total Vert"),
	TotalTri UMETA(DisplayName = "Total Tris")
};

UCLASS()
class AURA_API AGpuOptimizationHelper : public AActor {
	GENERATED_BODY()

public:
	//================================================================================================================
	// FUNCTIONS
	//================================================================================================================
#if WITH_EDITOR
	/**
	 *  Dumps all static mesh data in the current level to the output log
	 *  Data contains:
	 *  - Static Mesh Name
	 *  - Vertices Count
	 *  - Triangles Count
	 *  - MIN LOD Value
	 *  - Instances Count
	 *  - Total Vertices Count (Vertices Count * Instances Count)
	 *  - Total Triangles Count (Triangles Count * Instances Count)
	 */
	UFUNCTION(CallInEditor, Category = "Debug")
	void DumpAllStaticMeshData() const;
#endif
	
	//================================================================================================================
	// PROPERTIES & VARIABLES
	//================================================================================================================
#if WITH_EDITORONLY_DATA
	//  Type by which to sort the dumped static mesh data
	UPROPERTY(EditAnywhere, Category = "Debug")
	ESortKey SortBy = ESortKey::TotalVert;
	//  Whether to sort in descending order (highest to lowest) or ascending order (lowest to highest)
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bSortDescending = true;
	
	/**
	 *  The maximum value of MIN LOD Count to dump for each static mesh, 
	 *  if a static mesh has more than this value, it will be ignored in the dump
	 *  -1 means no limit
	 */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "-1", UIMin = "-1", ClampMax = "8", UIMax = "8"))
	int8 MaxMinLodCountToDump = 5;
	
	/**
	 *  The minimum value of Instances Count to dump for each static mesh,
	 *  if a static mesh has less than this value, it will be ignored in the dump
	 */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "1", UIMin = "1"))
	uint16 MinCountOfInstancesToDump = 1;
	
	/**
	 *  The minimum value of Vertices Count to dump for each static mesh,
	 *  if a static mesh has less than this value, it will be ignored in the dump
	 */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "1", UIMin = "1"))
	uint32 MinVertCountToDump = 1;
	
	/**
	 *  The minimum value of Total Vertices Count to dump for each static mesh,
	 *  if a static mesh has less than this value, it will be ignored in the dump
	 */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "1", UIMin = "1"))
	uint32 MinTotalVertCountToDump = 1;
	
	/**
	 *  The maximum number of entries to dump, to avoid spamming the output log
	 */
	UPROPERTY(EditAnywhere, Category = "Debug", meta = (ClampMin = "1", UIMin = "1", ClampMax = "255", UIMax = "255"))
	uint8 MaxEntriesToDump = 255;
#endif
	
};
	