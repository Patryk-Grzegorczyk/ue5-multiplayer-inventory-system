#pragma once

#include "CoreMinimal.h"
#include "Serialization/Archive.h"
#include "InventoryTypes.generated.h"

/** Logical inventory grids used by the sample. */
UENUM(BlueprintType)
enum class EInventoryGridType : uint8
{
    Backpack,
    Pocket,
    Storage
};

/** Small, data-oriented item category used by the inventory sample. */
UENUM(BlueprintType)
enum class EInventoryItemType : uint8
{
    Regular,
    Resource,
    Tool,
    Valuable
};

/**
 * Runtime item instance stored by the inventory component.
 *
 * The original DeepAnomaly item structure contains additional game-specific
 * data for tools, clothing, batteries, anomalies, UI and economy. Those
 * fields are intentionally omitted here so the sample focuses on inventory
 * state and grid placement.
 */
USTRUCT(BlueprintType)
struct FInventoryItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ItemID = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ItemName;

    /** Width/height measured in inventory cells. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint Size = FIntPoint(1, 1);

    /** Top-left cell occupied by the item. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint Position = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EInventoryItemType ItemType = EInventoryItemType::Regular;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRotated = false;

    bool operator==(const FInventoryItem& Other) const
    {
        return ItemID == Other.ItemID && Position == Other.Position;
    }

    /** Custom serialization used by replicated inventory grids. */
    bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << ItemID;
        Ar << ItemName;
        Ar << Size;
        Ar << Position;
        Ar << ItemType;
        Ar << bRotated;

        bOutSuccess = true;
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FInventoryItem> : public TStructOpsTypeTraitsBase2<FInventoryItem>
{
    enum
    {
        WithNetSerializer = true
    };
};

/** A rectangular inventory grid containing variable-size items. */
USTRUCT(BlueprintType)
struct FInventoryGrid
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntPoint GridSize = FIntPoint(6, 8);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInventoryItem> Items;

    bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << GridSize;
        Ar << Items;

        bOutSuccess = true;
        return true;
    }
};

template<>
struct TStructOpsTypeTraits<FInventoryGrid> : public TStructOpsTypeTraitsBase2<FInventoryGrid>
{
    enum
    {
        WithNetSerializer = true
    };
};
