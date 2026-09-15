#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

    BackpackGrid.GridSize = FIntPoint(6, 8);
    PocketGrid.GridSize = FIntPoint(2, 2);
    StorageGrid.GridSize = FIntPoint(8, 8);
}

void UInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UInventoryComponent, BackpackGrid);
    DOREPLIFETIME(UInventoryComponent, PocketGrid);
    DOREPLIFETIME(UInventoryComponent, StorageGrid);
}

const FInventoryGrid* UInventoryComponent::GetGrid(EInventoryGridType GridType) const
{
    switch (GridType)
    {
    case EInventoryGridType::Backpack:
        return &BackpackGrid;
    case EInventoryGridType::Pocket:
        return &PocketGrid;
    case EInventoryGridType::Storage:
        return &StorageGrid;
    default:
        return nullptr;
    }
}

FInventoryGrid* UInventoryComponent::GetMutableGrid(EInventoryGridType GridType)
{
    switch (GridType)
    {
    case EInventoryGridType::Backpack:
        return &BackpackGrid;
    case EInventoryGridType::Pocket:
        return &PocketGrid;
    case EInventoryGridType::Storage:
        return &StorageGrid;
    default:
        return nullptr;
    }
}

int32 UInventoryComponent::FindItemIndex(const FInventoryGrid& Grid, FIntPoint Position) const
{
    for (int32 Index = 0; Index < Grid.Items.Num(); ++Index)
    {
        if (Grid.Items[Index].Position == Position)
        {
            return Index;
        }
    }

    return INDEX_NONE;
}

bool UInventoryComponent::CanFitItemInPosition(const FInventoryGrid& Grid,
                                                FIntPoint ItemSize,
                                                FIntPoint Position) const
{
    if (ItemSize.X <= 0 || ItemSize.Y <= 0)
    {
        return false;
    }

    return Position.X >= 0 &&
           Position.Y >= 0 &&
           Position.X + ItemSize.X <= Grid.GridSize.X &&
           Position.Y + ItemSize.Y <= Grid.GridSize.Y;
}

bool UInventoryComponent::RectanglesOverlap(const FIntPoint& PosA,
                                            const FIntPoint& SizeA,
                                            const FIntPoint& PosB,
                                            const FIntPoint& SizeB) const
{
    if (PosA.X + SizeA.X <= PosB.X || PosB.X + SizeB.X <= PosA.X)
    {
        return false;
    }

    if (PosA.Y + SizeA.Y <= PosB.Y || PosB.Y + SizeB.Y <= PosA.Y)
    {
        return false;
    }

    return true;
}

bool UInventoryComponent::CanPlaceItemAtPosition(const FInventoryGrid& Grid,
                                                  const FInventoryItem& Item,
                                                  FIntPoint Position,
                                                  int32 IgnoredItemIndex) const
{
    if (!CanFitItemInPosition(Grid, Item.Size, Position))
    {
        return false;
    }

    for (int32 Index = 0; Index < Grid.Items.Num(); ++Index)
    {
        if (Index == IgnoredItemIndex)
        {
            continue;
        }

        const FInventoryItem& ExistingItem = Grid.Items[Index];

        if (RectanglesOverlap(Position, Item.Size, ExistingItem.Position, ExistingItem.Size))
        {
            return false;
        }
    }

    return true;
}

bool UInventoryComponent::CanPlaceItemInGrid(const FInventoryGrid& Grid,
                                             const FInventoryItem& NewItem,
                                             FIntPoint& OutPosition) const
{
    if (NewItem.Size.X <= 0 || NewItem.Size.Y <= 0)
    {
        return false;
    }

    // Same scan order as the original implementation: top-to-bottom, left-to-right.
    for (int32 Y = 0; Y <= Grid.GridSize.Y - NewItem.Size.Y; ++Y)
    {
        for (int32 X = 0; X <= Grid.GridSize.X - NewItem.Size.X; ++X)
        {
            const FIntPoint TestPosition(X, Y);

            if (CanPlaceItemAtPosition(Grid, NewItem, TestPosition))
            {
                OutPosition = TestPosition;
                return true;
            }
        }
    }

    return false;
}

bool UInventoryComponent::TryAddItemToGrid(FInventoryGrid& Grid, FInventoryItem NewItem)
{
    FIntPoint FoundPosition;

    if (!CanPlaceItemInGrid(Grid, NewItem, FoundPosition))
    {
        return false;
    }

    NewItem.Position = FoundPosition;
    Grid.Items.Add(NewItem);
    return true;
}

bool UInventoryComponent::AddItem(const FInventoryItem& NewItem, EInventoryGridType GridType)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        ServerAddItem(NewItem, GridType);
        return true;
    }

    FInventoryGrid* TargetGrid = GetMutableGrid(GridType);
    if (!TargetGrid)
    {
        return false;
    }

    if (!TryAddItemToGrid(*TargetGrid, NewItem))
    {
        return false;
    }

    NotifyInventoryChanged();
    return true;
}

void UInventoryComponent::ServerAddItem_Implementation(const FInventoryItem& NewItem,
                                                        EInventoryGridType GridType)
{
    // The server is the only authority that mutates replicated inventory state.
    FInventoryGrid* TargetGrid = GetMutableGrid(GridType);
    if (!TargetGrid)
    {
        return;
    }

    if (TryAddItemToGrid(*TargetGrid, NewItem))
    {
        NotifyInventoryChanged();
    }
}

bool UInventoryComponent::RemoveItemAtPosition(EInventoryGridType GridType, FIntPoint Position)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        ServerRemoveItemAtPosition(GridType, Position);
        return true;
    }

    FInventoryGrid* Grid = GetMutableGrid(GridType);
    if (!Grid)
    {
        return false;
    }

    const int32 ItemIndex = FindItemIndex(*Grid, Position);
    if (ItemIndex == INDEX_NONE)
    {
        return false;
    }

    Grid->Items.RemoveAt(ItemIndex);
    NotifyInventoryChanged();
    return true;
}

bool UInventoryComponent::ServerRemoveItemAtPosition_Validate(EInventoryGridType GridType,
                                                               FIntPoint Position)
{
    return Position.X >= 0 && Position.Y >= 0 && GetMutableGrid(GridType) != nullptr;
}

void UInventoryComponent::ServerRemoveItemAtPosition_Implementation(EInventoryGridType GridType,
                                                                      FIntPoint Position)
{
    FInventoryGrid* Grid = GetMutableGrid(GridType);
    if (!Grid)
    {
        return;
    }

    const int32 ItemIndex = FindItemIndex(*Grid, Position);
    if (ItemIndex == INDEX_NONE)
    {
        return;
    }

    Grid->Items.RemoveAt(ItemIndex);
    NotifyInventoryChanged();
}

bool UInventoryComponent::MoveItem(EInventoryGridType FromGrid,
                                   EInventoryGridType ToGrid,
                                   FIntPoint CurrentPosition,
                                   FIntPoint NewPosition)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        ServerMoveItem(FromGrid, ToGrid, CurrentPosition, NewPosition);
        return true;
    }

    return MoveItem_Internal(FromGrid, ToGrid, CurrentPosition, NewPosition);
}

bool UInventoryComponent::MoveItem_Internal(EInventoryGridType FromGridType,
                                            EInventoryGridType ToGridType,
                                            FIntPoint CurrentPosition,
                                            FIntPoint NewPosition)
{
    FInventoryGrid* SourceGrid = GetMutableGrid(FromGridType);
    FInventoryGrid* TargetGrid = GetMutableGrid(ToGridType);

    if (!SourceGrid || !TargetGrid)
    {
        return false;
    }

    const int32 SourceIndex = FindItemIndex(*SourceGrid, CurrentPosition);
    if (SourceIndex == INDEX_NONE)
    {
        return false;
    }

    FInventoryItem ItemToMove = SourceGrid->Items[SourceIndex];
    ItemToMove.Position = NewPosition;

    // Remove first only when moving inside the same grid. The ignored index
    // allows the item's current occupied cells to be excluded from collision tests.
    const int32 IgnoredIndex = (SourceGrid == TargetGrid) ? SourceIndex : INDEX_NONE;

    if (!CanPlaceItemAtPosition(*TargetGrid, ItemToMove, NewPosition, IgnoredIndex))
    {
        return false;
    }

    if (SourceGrid == TargetGrid)
    {
        SourceGrid->Items[SourceIndex] = ItemToMove;
    }
    else
    {
        SourceGrid->Items.RemoveAt(SourceIndex);
        TargetGrid->Items.Add(ItemToMove);
    }

    NotifyInventoryChanged();
    return true;
}

bool UInventoryComponent::ServerMoveItem_Validate(EInventoryGridType FromGrid,
                                                    EInventoryGridType ToGrid,
                                                    FIntPoint CurrentPosition,
                                                    FIntPoint NewPosition)
{
    return GetMutableGrid(FromGrid) != nullptr &&
           GetMutableGrid(ToGrid) != nullptr &&
           CurrentPosition.X >= 0 && CurrentPosition.Y >= 0 &&
           NewPosition.X >= 0 && NewPosition.Y >= 0;
}

void UInventoryComponent::ServerMoveItem_Implementation(EInventoryGridType FromGrid,
                                                          EInventoryGridType ToGrid,
                                                          FIntPoint CurrentPosition,
                                                          FIntPoint NewPosition)
{
    MoveItem_Internal(FromGrid, ToGrid, CurrentPosition, NewPosition);
}

bool UInventoryComponent::RotateItem(EInventoryGridType GridType, FIntPoint CurrentPosition)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        ServerRotateItem(GridType, CurrentPosition);
        return true;
    }

    return RotateItem_Internal(GridType, CurrentPosition);
}

bool UInventoryComponent::RotateItem_Internal(EInventoryGridType GridType, FIntPoint CurrentPosition)
{
    FInventoryGrid* Grid = GetMutableGrid(GridType);
    if (!Grid)
    {
        return false;
    }

    const int32 ItemIndex = FindItemIndex(*Grid, CurrentPosition);
    if (ItemIndex == INDEX_NONE)
    {
        return false;
    }

    FInventoryItem& Item = Grid->Items[ItemIndex];
    const FIntPoint RotatedSize(Item.Size.Y, Item.Size.X);

    FInventoryItem TestItem = Item;
    TestItem.Size = RotatedSize;

    if (!CanPlaceItemAtPosition(*Grid, TestItem, Item.Position, ItemIndex))
    {
        return false;
    }

    Item.Size = RotatedSize;
    Item.bRotated = !Item.bRotated;

    NotifyInventoryChanged();
    return true;
}

bool UInventoryComponent::ServerRotateItem_Validate(EInventoryGridType GridType,
                                                      FIntPoint CurrentPosition)
{
    return GetMutableGrid(GridType) != nullptr &&
           CurrentPosition.X >= 0 && CurrentPosition.Y >= 0;
}

void UInventoryComponent::ServerRotateItem_Implementation(EInventoryGridType GridType,
                                                            FIntPoint CurrentPosition)
{
    RotateItem_Internal(GridType, CurrentPosition);
}

void UInventoryComponent::OnRep_BackpackGrid()
{
    NotifyInventoryChanged();
}

void UInventoryComponent::OnRep_PocketGrid()
{
    NotifyInventoryChanged();
}

void UInventoryComponent::OnRep_StorageGrid()
{
    NotifyInventoryChanged();
}

void UInventoryComponent::NotifyInventoryChanged()
{
    OnInventoryChanged.Broadcast();
}
