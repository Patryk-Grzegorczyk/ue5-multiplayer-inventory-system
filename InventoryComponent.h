#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "InventoryComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnInventoryChanged);

/**
 * Replicated grid-based inventory extracted and simplified from DeepAnomaly.
 *
 * Responsibilities intentionally limited to:
 * - variable-size item placement
 * - rotation
 * - moving items between supported grids
 * - server-authoritative mutations
 * - replicated inventory state
 * - notifying presentation code when state changes
 */
UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInventoryComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Adds an item to the first available position in the requested grid. */
    bool AddItem(const FInventoryItem& NewItem, EInventoryGridType GridType = EInventoryGridType::Backpack);

    /** Removes the item whose top-left cell matches Position. */
    bool RemoveItemAtPosition(EInventoryGridType GridType, FIntPoint Position);

    /** Attempts to move an item to an exact destination cell. */
    bool MoveItem(EInventoryGridType FromGrid,
                  EInventoryGridType ToGrid,
                  FIntPoint CurrentPosition,
                  FIntPoint NewPosition);

    /** Rotates an item in-place. The operation is accepted only if the rotated shape fits. */
    bool RotateItem(EInventoryGridType GridType, FIntPoint CurrentPosition);

    /** Returns the grid used by the requested logical inventory section. */
    const FInventoryGrid* GetGrid(EInventoryGridType GridType) const;

    /** Event for UI/presentation code. The component does not own widgets. */
    FOnInventoryChanged OnInventoryChanged;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(ReplicatedUsing=OnRep_BackpackGrid, EditAnywhere, BlueprintReadOnly, Category="Inventory")
    FInventoryGrid BackpackGrid;

    UPROPERTY(ReplicatedUsing=OnRep_PocketGrid, EditAnywhere, BlueprintReadOnly, Category="Inventory")
    FInventoryGrid PocketGrid;

    UPROPERTY(ReplicatedUsing=OnRep_StorageGrid, EditAnywhere, BlueprintReadOnly, Category="Inventory")
    FInventoryGrid StorageGrid;

    UFUNCTION()
    void OnRep_BackpackGrid();

    UFUNCTION()
    void OnRep_PocketGrid();

    UFUNCTION()
    void OnRep_StorageGrid();

    UFUNCTION(Server, Reliable)
    void ServerAddItem(const FInventoryItem& NewItem, EInventoryGridType GridType);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerMoveItem(EInventoryGridType FromGrid,
                        EInventoryGridType ToGrid,
                        FIntPoint CurrentPosition,
                        FIntPoint NewPosition);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRemoveItemAtPosition(EInventoryGridType GridType, FIntPoint Position);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRotateItem(EInventoryGridType GridType, FIntPoint CurrentPosition);

    bool TryAddItemToGrid(FInventoryGrid& Grid, FInventoryItem NewItem);
    bool CanPlaceItemInGrid(const FInventoryGrid& Grid,
                            const FInventoryItem& NewItem,
                            FIntPoint& OutPosition) const;
    bool CanFitItemInPosition(const FInventoryGrid& Grid,
                              FIntPoint ItemSize,
                              FIntPoint Position) const;
    bool CanPlaceItemAtPosition(const FInventoryGrid& Grid,
                                const FInventoryItem& Item,
                                FIntPoint Position,
                                int32 IgnoredItemIndex = INDEX_NONE) const;

    bool RectanglesOverlap(const FIntPoint& PosA,
                           const FIntPoint& SizeA,
                           const FIntPoint& PosB,
                           const FIntPoint& SizeB) const;

    FInventoryGrid* GetMutableGrid(EInventoryGridType GridType);

    int32 FindItemIndex(const FInventoryGrid& Grid, FIntPoint Position) const;

    /** Server-side implementation shared by local calls and RPCs. */
    bool MoveItem_Internal(EInventoryGridType FromGrid,
                           EInventoryGridType ToGrid,
                           FIntPoint CurrentPosition,
                           FIntPoint NewPosition);

    /** Server-side implementation shared by local calls and RPCs. */
    bool RotateItem_Internal(EInventoryGridType GridType, FIntPoint CurrentPosition);

    void NotifyInventoryChanged();
};
