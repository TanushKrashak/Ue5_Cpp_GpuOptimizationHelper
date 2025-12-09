#include "GpuOptimizationHelper.h"

#if WITH_EDITOR
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"

struct FMeshInfo {
    int32 MinLOD = 0;
    uint32 VertCount = 0;
    uint32 TriCount = 0;
    uint32 Count = 1;
    FString ShortPath;

    FORCEINLINE int64 TotalVerts() const {
        return static_cast<int64>(VertCount) * Count;
    }
    FORCEINLINE int64 TotalTris() const {
        return static_cast<int64>(TriCount) * Count;
    }

    FMeshInfo(const int32 InMinLOD, const uint32 InVerts, const uint32 InTris, FString InShortPath)
        : MinLOD(InMinLOD), VertCount(InVerts), TriCount(InTris), ShortPath(MoveTemp(InShortPath)) {
    }
};

void AGpuOptimizationHelper::DumpAllStaticMeshData() const {
    UE_LOG(LogTemp, Warning, TEXT("=== Dumping Static Mesh Stats (World Only) ==="));
    const UWorld* World = GetWorld();
    if (!World) {
        UE_LOG(LogTemp, Error, TEXT("World invalid."));
        return;
    }
    TMap<FName, FMeshInfo> MeshMap;
    int64 GlobalTotalVerts = 0;
    int64 GlobalTotalTris = 0;

    // Widths
    uint8 NameWidth = 4;
    uint8 MinLODWidth = 6;
    uint8 VertWidth = 5;
    uint8 TriWidth = 5;
    uint8 CountWidth = 5;
    uint8 TotalVertWidth = 10;
    uint8 TotalTriWidth = 9;
    uint16 PathWidth = 10;

    const FString KeyFolder = TEXT("Assets/MapBuildingAssets/");
    for (TActorIterator<AActor> It(World); It; ++It) {
        const AActor* Actor = *It;
        TArray<UStaticMeshComponent*> Components;
        Actor->GetComponents<UStaticMeshComponent>(Components);

        for (const UStaticMeshComponent* SMC : Components) {
            if (!SMC) {
                continue;
            }
            UStaticMesh* Mesh = SMC->GetStaticMesh();
            if (!Mesh || !Mesh->GetRenderData()) {
                continue;
            }
            const int32 MinLOD = Mesh->GetMinLOD().GetValue();
            const int32 LODToUse = FMath::Clamp(MinLOD, 0, Mesh->GetNumLODs() - 1);
            if (MaxMinLodCountToDump >= 0 && LODToUse > MaxMinLodCountToDump) { // Filter by Min LOD
                continue;
            }
            const FStaticMeshLODResources& LODRes = Mesh->GetRenderData()->LODResources[LODToUse];

            const uint32 VertCount = LODRes.GetNumVertices();
            if (VertCount < MinVertCountToDump) { // Filter by Vert Count
                continue;
            }
            const uint32 TriCount = LODRes.GetNumTriangles();
            GlobalTotalVerts += VertCount;
            GlobalTotalTris += TriCount;

            FString AssetPath = Mesh->GetPathName();
            const int32 Index = AssetPath.Find(KeyFolder);
            FString ShortPath = (Index != INDEX_NONE) ? AssetPath.Mid(Index + KeyFolder.Len()) : AssetPath;

            const FName Name = Mesh->GetFName();
            // Update Width
            auto UpdateWidth = [](uint8& Width, const int64 Val) {
                Width = FMath::Max(Width, static_cast<uint8>(FString::Printf(TEXT("%lld"), Val).Len()));
                };
            if (MeshMap.Contains(Name)) { // update existing entry
                MeshMap[Name].Count += 1;
                CountWidth = FMath::Max(CountWidth, static_cast<uint8>(FString::FromInt(MeshMap[Name].Count).Len()));
                UpdateWidth(TotalVertWidth, MeshMap[Name].TotalVerts());
                UpdateWidth(TotalTriWidth, MeshMap[Name].TotalTris());
            }
            else { // new entry
                FMeshInfo Info(MinLOD, VertCount, TriCount, ShortPath);
                MeshMap.Emplace(Name, Info);

                NameWidth = FMath::Max(NameWidth, static_cast<uint8>(Name.ToString().Len()));
                UpdateWidth(MinLODWidth, MinLOD);
                UpdateWidth(VertWidth, VertCount);
                UpdateWidth(TriWidth, TriCount);
                TotalVertWidth = FMath::Max(TotalVertWidth, VertWidth);
                TotalTriWidth = FMath::Max(TotalTriWidth, TriWidth);
                PathWidth = FMath::Max(PathWidth, static_cast<uint16>(ShortPath.Len()));
            }
        }
    }

    TArray<TPair<FName, FMeshInfo>> MeshArray;
    for (const auto& Elem : MeshMap) {
        if (Elem.Value.Count < MinCountOfInstancesToDump) { // Filter by instance count
            continue;
        }
        if (Elem.Value.TotalVerts() < MinTotalVertCountToDump) { // Filter by total vert count
            continue;
        }
        MeshArray.Add(Elem);
    }
    // Sort
    auto SortFunc = [&](const auto& A, const auto& B) {
        // Name sorting needs its own branch
        if (SortBy == ESortKey::Name) {
            const bool bResult = A.Key.LexicalLess(B.Key);
            return bSortDescending ? !bResult : bResult;
        }

        auto GetKey = [&](const auto& Elem) -> int64 {
            auto& Val = Elem.Value;
            switch (SortBy) {
            case ESortKey::Vert:
                return Val.VertCount;
            case ESortKey::Tri:
                return Val.TriCount;
            case ESortKey::Count:
                return Val.Count;
            case ESortKey::TotalVert:
                return Val.TotalVerts();
            case ESortKey::TotalTri:
                return Val.TotalTris();
            case ESortKey::MinLOD:
                return Val.MinLOD;
            default:
                return 0;
            }
            };
        const int64 KeyA = GetKey(A);
        const int64 KeyB = GetKey(B);
        return bSortDescending ? (KeyA > KeyB) : (KeyA < KeyB);
        };
    MeshArray.Sort(SortFunc);

    // Print
    {
        // Table Header
        UE_LOG(LogTemp, Warning,
            TEXT("%*s | %*s | %*s | %*s | %*s | %*s | %*s | %-*s"),
            NameWidth, TEXT("Mesh"),
            MinLODWidth, TEXT("MinLOD"),
            VertWidth, TEXT("Verts"),
            TriWidth, TEXT("Tris"),
            CountWidth, TEXT("Count"),
            TotalVertWidth, TEXT("TotalVerts"),
            TotalTriWidth, TEXT("TotalTris"),
            PathWidth, TEXT("Path"));
        // Table Data
        uint8 EntriesDumped = 0;
        for (const auto& Elem : MeshArray) {
            const FMeshInfo& Info = Elem.Value;
            UE_LOG(LogTemp, Warning,
                TEXT("%*s | %*d | %*d | %*d | %*d | %*lld | %*lld | %-*s"),
                NameWidth, *Elem.Key.ToString(),
                MinLODWidth, Info.MinLOD,
                VertWidth, Info.VertCount,
                TriWidth, Info.TriCount,
                CountWidth, Info.Count,
                TotalVertWidth, Info.TotalVerts(),
                TotalTriWidth, Info.TotalTris(),
                PathWidth, *Info.ShortPath);
            EntriesDumped++;
            if (EntriesDumped >= MaxEntriesToDump) {
                break;
            }
        }

        UE_LOG(LogTemp, Warning,
            TEXT("=== GLOBAL TOTAL Verts: %lld   TOTAL Tris: %lld ==="),
            GlobalTotalVerts, GlobalTotalTris);
    }
}
#endif
