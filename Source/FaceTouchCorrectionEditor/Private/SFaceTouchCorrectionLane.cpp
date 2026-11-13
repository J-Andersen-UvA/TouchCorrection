#include "SFaceTouchCorrectionLane.h"

#include "Animation/AnimSequenceBase.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"

namespace
{
constexpr float pixelsPerSecond = 180.0f;
constexpr float laneHeight = 70.0f;
constexpr float blockHeight = 58.0f;
constexpr float handleRadius = 7.0f;
constexpr float edgeHitWidth = 7.0f;
constexpr int32 curveSampleCount = 32;

void paintTriangle(
    const FVector2D& first,
    const FVector2D& second,
    const FVector2D& third,
    const FGeometry& geometry,
    const FLinearColor& color,
    const FSlateBrush& brush,
    FSlateWindowElementList& drawElements,
    const int32 layerId,
    const ESlateDrawEffect drawEffects)
{
    TArray<FSlateVertex> vertices;
    vertices.Reserve(3);
    for (const FVector2D& point : {first, second, third})
    {
        FSlateVertex& vertex = vertices.AddDefaulted_GetRef();
        vertex.Position = FVector2f(geometry.LocalToAbsolute(point));
        vertex.Color = color.ToFColor(false);
    }
    const TArray<SlateIndex> indices = {0, 1, 2};
    FSlateDrawElement::MakeCustomVerts(
        drawElements,
        layerId,
        brush.GetRenderingResource(),
        vertices,
        indices,
        nullptr,
        0,
        0,
        drawEffects);
}
}

void SFaceTouchCorrectionLane::Construct(const FArguments& arguments)
{
    onCorrectionSelected = arguments._OnCorrectionSelected;
    onCorrectionEditRequested = arguments._OnCorrectionEditRequested;
    onCorrectionChanged = arguments._OnCorrectionChanged;
    SetCanTick(false);
}

void SFaceTouchCorrectionLane::setTimeline(
    UAnimSequenceBase* newAnimation,
    const TArray<FFaceTouchCorrectionBlock>& newCorrections,
    const int32 newSelectedCorrectionIndex,
    const float newZoom)
{
    animation = newAnimation;
    corrections = newCorrections;
    selectedCorrectionIndex = newSelectedCorrectionIndex;
    zoom = newZoom;
    Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

FVector2D SFaceTouchCorrectionLane::ComputeDesiredSize(float) const
{
    return FVector2D(FMath::Max(1.0f, getTotalWidth()), laneHeight);
}

int32 SFaceTouchCorrectionLane::OnPaint(
    const FPaintArgs& arguments,
    const FGeometry& allottedGeometry,
    const FSlateRect& clippingRect,
    FSlateWindowElementList& outDrawElements,
    const int32 layerId,
    const FWidgetStyle& widgetStyle,
    const bool parentEnabled) const
{
    const FSlateBrush* brush = FAppStyle::GetBrush(TEXT("WhiteBrush"));
    const ESlateDrawEffect drawEffects = parentEnabled
        ? ESlateDrawEffect::None
        : ESlateDrawEffect::DisabledEffect;

    FSlateDrawElement::MakeBox(
        outDrawElements,
        layerId,
        allottedGeometry.ToPaintGeometry(
            FVector2D(FMath::Max(1.0f, getTotalWidth()), blockHeight),
            FSlateLayoutTransform()),
        brush,
        drawEffects,
        FLinearColor(0.07f, 0.07f, 0.07f, 1.0f));

    for (int32 correctionIndex = 0; correctionIndex < corrections.Num(); ++correctionIndex)
    {
        const FFaceTouchCorrectionBlock& correction = corrections[correctionIndex];
        if (!correction.isValid())
        {
            continue;
        }

        const float left = getFrameOffset(correction.approachZeroFrame);
        const float right = getFrameOffset(
            static_cast<double>(correction.recoveryZeroFrame) + 1.0);
        const float width = FMath::Max(2.0f, right - left);
        FLinearColor color = correctionIndex % 2 == 0
            ? FLinearColor(0.34f, 0.29f, 0.45f, 1.0f)
            : FLinearColor(0.24f, 0.36f, 0.44f, 1.0f);
        color.A = correctionIndex == selectedCorrectionIndex ? 0.96f : 0.78f;

        FSlateDrawElement::MakeBox(
            outDrawElements,
            layerId + 1,
            allottedGeometry.ToPaintGeometry(
                FVector2D(width, blockHeight),
                FSlateLayoutTransform(FVector2D(left, 0.0f))),
            brush,
            drawEffects,
            color);

        if (correctionIndex == selectedCorrectionIndex)
        {
            const FLinearColor selectionColor(0.0f, 0.72f, 0.9f, 1.0f);
            for (const TPair<FVector2D, FVector2D>& border : {
                     TPair<FVector2D, FVector2D>(
                         FVector2D(left, 0.0f), FVector2D(width, 2.0f)),
                     TPair<FVector2D, FVector2D>(
                         FVector2D(left, blockHeight - 2.0f), FVector2D(width, 2.0f)),
                     TPair<FVector2D, FVector2D>(
                         FVector2D(left, 0.0f), FVector2D(2.0f, blockHeight)),
                     TPair<FVector2D, FVector2D>(
                         FVector2D(right - 2.0f, 0.0f), FVector2D(2.0f, blockHeight))})
            {
                FSlateDrawElement::MakeBox(
                    outDrawElements,
                    layerId + 2,
                    allottedGeometry.ToPaintGeometry(
                        border.Value,
                        FSlateLayoutTransform(border.Key)),
                    brush,
                    drawEffects,
                    selectionColor);
            }
        }

        TArray<FVector2D> curvePoints;
        curvePoints.Reserve(curveSampleCount + 1);
        const double frameRange = FMath::Max(
            1.0,
            static_cast<double>(
                correction.recoveryZeroFrame - correction.approachZeroFrame));
        for (int32 sampleIndex = 0; sampleIndex <= curveSampleCount; ++sampleIndex)
        {
            const double alpha = static_cast<double>(sampleIndex) / curveSampleCount;
            const double frame = correction.approachZeroFrame + frameRange * alpha;
            curvePoints.Add(FVector2D(
                getFrameOffset(frame),
                FMath::Lerp(33.0f, 5.0f, correction.evaluateWeight(frame))));
        }
        FSlateDrawElement::MakeLines(
            outDrawElements,
            layerId + 3,
            allottedGeometry.ToPaintGeometry(),
            curvePoints,
            drawEffects,
            FLinearColor::White,
            true,
            1.5f);

        FSlateDrawElement::MakeText(
            outDrawElements,
            layerId + 4,
            allottedGeometry.ToPaintGeometry(
                FVector2D(FMath::Max(1.0f, width - 12.0f), 18.0f),
                FSlateLayoutTransform(FVector2D(left + 6.0f, 38.0f))),
            correction.getDisplayName(),
            FAppStyle::GetFontStyle(TEXT("SmallFont")),
            drawEffects,
            FLinearColor::White);

        for (const TPair<int32, EDragTarget>& handle : {
                 TPair<int32, EDragTarget>(
                     correction.touchStartFrame,
                     EDragTarget::TouchStart),
                 TPair<int32, EDragTarget>(
                     correction.touchEndFrame,
                     EDragTarget::TouchEnd)})
        {
            const float handleX = getFrameOffset(handle.Key);
            const bool isHovered = hoveredHit.correctionIndex == correctionIndex &&
                hoveredHit.target == handle.Value;
            const float handleSize = isHovered ? 10.0f : 8.0f;
            const float halfSize = handleSize * 0.5f;
            const bool pointsLeft = handle.Value == EDragTarget::TouchStart;
            const FVector2D tip(
                handleX + (pointsLeft ? -halfSize : halfSize),
                halfSize);
            const float baseX = handleX + (pointsLeft ? halfSize : -halfSize);
            paintTriangle(
                tip,
                FVector2D(baseX, 0.0f),
                FVector2D(baseX, handleSize),
                allottedGeometry,
                isHovered
                    ? FLinearColor(0.96f, 0.96f, 1.0f, 1.0f)
                    : FLinearColor(0.86f, 0.86f, 0.9f, 1.0f),
                *brush,
                outDrawElements,
                layerId + 5,
                drawEffects);
        }
        for (const TPair<float, EDragTarget>& edge : {
                 TPair<float, EDragTarget>(left, EDragTarget::ApproachZero),
                 TPair<float, EDragTarget>(right, EDragTarget::RecoveryZero)})
        {
            const bool isHovered = hoveredHit.correctionIndex == correctionIndex &&
                hoveredHit.target == edge.Value;
            const float edgeWidth = isHovered ? 3.0f : 2.0f;
            FSlateDrawElement::MakeBox(
                outDrawElements,
                layerId + 5,
                allottedGeometry.ToPaintGeometry(
                    FVector2D(edgeWidth, blockHeight),
                    FSlateLayoutTransform(FVector2D(
                        edge.Key - edgeWidth * 0.5f,
                        0.0f))),
                brush,
                drawEffects,
                isHovered
                    ? FLinearColor(0.94f, 0.94f, 0.98f, 1.0f)
                    : FLinearColor(0.8f, 0.8f, 0.84f, 1.0f));
        }
    }

    return layerId + 6;
}

FReply SFaceTouchCorrectionLane::OnMouseButtonDown(
    const FGeometry& myGeometry,
    const FPointerEvent& mouseEvent)
{
    const FKey mouseButton = mouseEvent.GetEffectingButton();
    if (mouseButton != EKeys::LeftMouseButton &&
        mouseButton != EKeys::RightMouseButton)
    {
        return FReply::Unhandled();
    }

    const FVector2D localPosition = myGeometry.AbsoluteToLocal(
        mouseEvent.GetScreenSpacePosition());
    const FHitResult hit = hitTest(localPosition.X, localPosition.Y);
    if (!corrections.IsValidIndex(hit.correctionIndex))
    {
        return FReply::Unhandled();
    }

    selectedCorrectionIndex = hit.correctionIndex;
    onCorrectionSelected.ExecuteIfBound(hit.correctionIndex);
    Invalidate(EInvalidateWidgetReason::Paint);
    if (mouseButton == EKeys::RightMouseButton)
    {
        onCorrectionEditRequested.ExecuteIfBound(hit.correctionIndex);
        return FReply::Handled();
    }

    draggedCorrectionIndex = hit.correctionIndex;
    dragTarget = hit.target;
    hoveredHit = hit;
    correctionAtDragStart = corrections[hit.correctionIndex];
    cursorFrameAtDragStart = getFrameAtOffset(localPosition.X);
    return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SFaceTouchCorrectionLane::OnMouseMove(
    const FGeometry& myGeometry,
    const FPointerEvent& mouseEvent)
{
    const FVector2D localPosition = myGeometry.AbsoluteToLocal(
        mouseEvent.GetScreenSpacePosition());
    if (!HasMouseCapture() || !corrections.IsValidIndex(draggedCorrectionIndex))
    {
        const FHitResult newHoveredHit = hitTest(localPosition.X, localPosition.Y);
        if (newHoveredHit.correctionIndex != hoveredHit.correctionIndex ||
            newHoveredHit.target != hoveredHit.target)
        {
            hoveredHit = newHoveredHit;
            Invalidate(EInvalidateWidgetReason::Paint);
        }
        return FReply::Unhandled();
    }
    updateDrag(
        localPosition.X,
        false);
    return FReply::Handled();
}

FReply SFaceTouchCorrectionLane::OnMouseButtonUp(
    const FGeometry& myGeometry,
    const FPointerEvent& mouseEvent)
{
    if (mouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !HasMouseCapture())
    {
        return FReply::Unhandled();
    }
    updateDrag(
        myGeometry.AbsoluteToLocal(mouseEvent.GetScreenSpacePosition()).X,
        true);
    draggedCorrectionIndex = INDEX_NONE;
    dragTarget = EDragTarget::None;
    const FVector2D localPosition = myGeometry.AbsoluteToLocal(
        mouseEvent.GetScreenSpacePosition());
    hoveredHit = hitTest(localPosition.X, localPosition.Y);
    Invalidate(EInvalidateWidgetReason::Paint);
    return FReply::Handled().ReleaseMouseCapture();
}

void SFaceTouchCorrectionLane::OnMouseLeave(const FPointerEvent& mouseEvent)
{
    SLeafWidget::OnMouseLeave(mouseEvent);
    if (!HasMouseCapture() && hoveredHit.correctionIndex != INDEX_NONE)
    {
        hoveredHit = {};
        Invalidate(EInvalidateWidgetReason::Paint);
    }
}

SFaceTouchCorrectionLane::FHitResult SFaceTouchCorrectionLane::hitTest(
    const float localX,
    const float localY) const
{
    if (localY < 0.0f || localY > blockHeight)
    {
        return {};
    }

    FHitResult closestEdge;
    float closestEdgeDistance = edgeHitWidth + 1.0f;
    for (int32 correctionIndex = 0; correctionIndex < corrections.Num(); ++correctionIndex)
    {
        const FFaceTouchCorrectionBlock& correction = corrections[correctionIndex];
        for (const TPair<float, EDragTarget>& edge : {
                 TPair<float, EDragTarget>(
                     getFrameOffset(correction.approachZeroFrame),
                     EDragTarget::ApproachZero),
                 TPair<float, EDragTarget>(
                     getFrameOffset(static_cast<double>(correction.recoveryZeroFrame) + 1.0),
                     EDragTarget::RecoveryZero)})
        {
            const float distance = FMath::Abs(localX - edge.Key);
            const bool winsTie = FMath::IsNearlyEqual(distance, closestEdgeDistance) &&
                correctionIndex == selectedCorrectionIndex;
            if (distance <= edgeHitWidth &&
                (distance < closestEdgeDistance || winsTie))
            {
                closestEdge = {correctionIndex, edge.Value};
                closestEdgeDistance = distance;
            }
        }
    }
    if (closestEdge.correctionIndex != INDEX_NONE)
    {
        return closestEdge;
    }

    if (localY <= 18.0f)
    {
        FHitResult closestHandle;
        float closestHandleDistance = handleRadius + 1.0f;
        for (int32 correctionIndex = 0; correctionIndex < corrections.Num(); ++correctionIndex)
        {
            const FFaceTouchCorrectionBlock& correction = corrections[correctionIndex];
            for (const TPair<float, EDragTarget>& handle : {
                     TPair<float, EDragTarget>(
                         getFrameOffset(correction.touchStartFrame),
                         EDragTarget::TouchStart),
                     TPair<float, EDragTarget>(
                         getFrameOffset(correction.touchEndFrame),
                         EDragTarget::TouchEnd)})
            {
                const float distance = FMath::Abs(localX - handle.Key);
                const bool winsTie = FMath::IsNearlyEqual(
                    distance,
                    closestHandleDistance) &&
                    correctionIndex == selectedCorrectionIndex;
                if (distance <= handleRadius &&
                    (distance < closestHandleDistance || winsTie))
                {
                    closestHandle = {correctionIndex, handle.Value};
                    closestHandleDistance = distance;
                }
            }
        }
        if (closestHandle.correctionIndex != INDEX_NONE)
        {
            return closestHandle;
        }
    }

    for (int32 correctionIndex = corrections.Num() - 1; correctionIndex >= 0; --correctionIndex)
    {
        const FFaceTouchCorrectionBlock& correction = corrections[correctionIndex];
        const float left = getFrameOffset(correction.approachZeroFrame);
        const float right = getFrameOffset(
            static_cast<double>(correction.recoveryZeroFrame) + 1.0);
        if (localX >= left && localX <= right)
        {
            return {correctionIndex, EDragTarget::Move};
        }
    }
    return {};
}

void SFaceTouchCorrectionLane::updateDrag(const float localX, const bool isFinal)
{
    if (!corrections.IsValidIndex(draggedCorrectionIndex))
    {
        return;
    }

    const int32 cursorFrame = getFrameAtOffset(localX);
    FFaceTouchCorrectionBlock updated = correctionAtDragStart;
    switch (dragTarget)
    {
    case EDragTarget::Move:
        updated.shiftBy(cursorFrame - cursorFrameAtDragStart, getLastFrame());
        break;
    case EDragTarget::ApproachZero:
        updated.approachZeroFrame = FMath::Clamp(
            cursorFrame,
            0,
            updated.touchStartFrame);
        break;
    case EDragTarget::TouchStart:
        updated.touchStartFrame = FMath::Clamp(
            cursorFrame,
            updated.approachZeroFrame,
            updated.touchEndFrame);
        break;
    case EDragTarget::TouchEnd:
        updated.touchEndFrame = FMath::Clamp(
            cursorFrame,
            updated.touchStartFrame,
            updated.recoveryZeroFrame);
        break;
    case EDragTarget::RecoveryZero:
        updated.recoveryZeroFrame = FMath::Clamp(
            cursorFrame,
            updated.touchEndFrame,
            getLastFrame());
        break;
    default:
        return;
    }

    corrections[draggedCorrectionIndex] = updated;
    onCorrectionChanged.ExecuteIfBound(
        draggedCorrectionIndex,
        updated,
        isFinal);
    Invalidate(EInvalidateWidgetReason::Paint);
}

float SFaceTouchCorrectionLane::getFrameOffset(const double frame) const
{
    const UAnimSequenceBase* loadedAnimation = animation.Get();
    if (loadedAnimation == nullptr || !loadedAnimation->GetSamplingFrameRate().IsValid())
    {
        return 0.0f;
    }
    return static_cast<float>(
        frame / loadedAnimation->GetSamplingFrameRate().AsDecimal()) *
        pixelsPerSecond * zoom;
}

int32 SFaceTouchCorrectionLane::getFrameAtOffset(const float offset) const
{
    const UAnimSequenceBase* loadedAnimation = animation.Get();
    if (loadedAnimation == nullptr || !loadedAnimation->GetSamplingFrameRate().IsValid())
    {
        return 0;
    }
    const double frame = offset / FMath::Max(pixelsPerSecond * zoom, 1.0f) *
        loadedAnimation->GetSamplingFrameRate().AsDecimal();
    return FMath::Clamp(FMath::RoundToInt(frame), 0, getLastFrame());
}

int32 SFaceTouchCorrectionLane::getLastFrame() const
{
    const UAnimSequenceBase* loadedAnimation = animation.Get();
    return loadedAnimation != nullptr
        ? FMath::Max(0, loadedAnimation->GetNumberOfSampledKeys() - 1)
        : 0;
}

float SFaceTouchCorrectionLane::getTotalWidth() const
{
    const UAnimSequenceBase* loadedAnimation = animation.Get();
    if (loadedAnimation == nullptr || !loadedAnimation->GetSamplingFrameRate().IsValid())
    {
        return 1.0f;
    }
    return static_cast<float>(loadedAnimation->GetSamplingFrameRate().AsSeconds(
        loadedAnimation->GetNumberOfSampledKeys())) * pixelsPerSecond * zoom;
}
