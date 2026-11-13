#pragma once

#include "FaceTouchCorrectionTypes.h"
#include "Widgets/SLeafWidget.h"

class UAnimSequenceBase;

DECLARE_DELEGATE_OneParam(FOnFaceTouchCorrectionSelected, int32)
DECLARE_DELEGATE_OneParam(FOnFaceTouchCorrectionEditRequested, int32)
DECLARE_DELEGATE_ThreeParams(
    FOnFaceTouchCorrectionChanged,
    int32,
    const FFaceTouchCorrectionBlock&,
    bool)

class FACETOUCHCORRECTIONEDITOR_API SFaceTouchCorrectionLane : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SFaceTouchCorrectionLane)
    {
    }
        SLATE_EVENT(FOnFaceTouchCorrectionSelected, OnCorrectionSelected)
        SLATE_EVENT(FOnFaceTouchCorrectionEditRequested, OnCorrectionEditRequested)
        SLATE_EVENT(FOnFaceTouchCorrectionChanged, OnCorrectionChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& arguments);
    void setTimeline(
        UAnimSequenceBase* animation,
        const TArray<FFaceTouchCorrectionBlock>& newCorrections,
        int32 newSelectedCorrectionIndex,
        float newZoom);

    virtual FVector2D ComputeDesiredSize(float layoutScaleMultiplier) const override;
    virtual int32 OnPaint(
        const FPaintArgs& arguments,
        const FGeometry& allottedGeometry,
        const FSlateRect& clippingRect,
        FSlateWindowElementList& outDrawElements,
        int32 layerId,
        const FWidgetStyle& widgetStyle,
        bool parentEnabled) const override;
    virtual FReply OnMouseButtonDown(
        const FGeometry& myGeometry,
        const FPointerEvent& mouseEvent) override;
    virtual FReply OnMouseMove(
        const FGeometry& myGeometry,
        const FPointerEvent& mouseEvent) override;
    virtual FReply OnMouseButtonUp(
        const FGeometry& myGeometry,
        const FPointerEvent& mouseEvent) override;
    virtual void OnMouseLeave(const FPointerEvent& mouseEvent) override;

private:
    enum class EDragTarget
    {
        None,
        Move,
        ApproachZero,
        TouchStart,
        TouchEnd,
        RecoveryZero
    };

    struct FHitResult
    {
        int32 correctionIndex = INDEX_NONE;
        EDragTarget target = EDragTarget::None;
    };

    FHitResult hitTest(float localX, float localY) const;
    void updateDrag(float localX, bool isFinal);
    float getFrameOffset(double frame) const;
    int32 getFrameAtOffset(float offset) const;
    int32 getLastFrame() const;
    float getTotalWidth() const;

    TWeakObjectPtr<UAnimSequenceBase> animation;
    TArray<FFaceTouchCorrectionBlock> corrections;
    int32 selectedCorrectionIndex = INDEX_NONE;
    int32 draggedCorrectionIndex = INDEX_NONE;
    EDragTarget dragTarget = EDragTarget::None;
    FHitResult hoveredHit;
    FFaceTouchCorrectionBlock correctionAtDragStart;
    int32 cursorFrameAtDragStart = 0;
    float zoom = 1.0f;
    FOnFaceTouchCorrectionSelected onCorrectionSelected;
    FOnFaceTouchCorrectionEditRequested onCorrectionEditRequested;
    FOnFaceTouchCorrectionChanged onCorrectionChanged;
};
