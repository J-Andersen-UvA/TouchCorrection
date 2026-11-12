#include "FaceTouchCorrectionTypes.h"

bool FFaceTouchSurfaceLocation::isValid() const
{
    return !skeletalMeshComponentName.IsNone();
}

FFaceTouchCorrectionBlock::FFaceTouchCorrectionBlock()
    : correctionId(FGuid::NewGuid())
{
}

bool FFaceTouchCorrectionBlock::isValid() const
{
    return approachZeroFrame >= 0 &&
        approachZeroFrame <= touchStartFrame &&
        touchStartFrame <= touchEndFrame &&
        touchEndFrame <= recoveryZeroFrame;
}

double FFaceTouchCorrectionBlock::evaluateWeight(const double frame) const
{
    if (!isValid() || frame <= approachZeroFrame || frame >= recoveryZeroFrame)
    {
        return 0.0;
    }
    if (frame >= touchStartFrame && frame <= touchEndFrame)
    {
        return 1.0;
    }

    const double rangeStart = frame < touchStartFrame
        ? approachZeroFrame
        : touchEndFrame;
    const double rangeEnd = frame < touchStartFrame
        ? touchStartFrame
        : recoveryZeroFrame;
    const double alpha = FMath::Clamp(
        (frame - rangeStart) / FMath::Max(rangeEnd - rangeStart, 1.0),
        0.0,
        1.0);
    const double smoothAlpha = alpha * alpha * (3.0 - 2.0 * alpha);
    return frame < touchStartFrame ? smoothAlpha : 1.0 - smoothAlpha;
}

void FFaceTouchCorrectionBlock::shiftBy(const int32 frameDelta, const int32 lastFrame)
{
    if (!isValid())
    {
        return;
    }
    const int32 minimumDelta = -approachZeroFrame;
    const int32 maximumDelta = lastFrame - recoveryZeroFrame;
    const int32 clampedDelta = FMath::Clamp(frameDelta, minimumDelta, maximumDelta);
    approachZeroFrame += clampedDelta;
    touchStartFrame += clampedDelta;
    touchEndFrame += clampedDelta;
    recoveryZeroFrame += clampedDelta;
}

FText FFaceTouchCorrectionBlock::getDisplayName() const
{
    if (interactionPointIds.IsEmpty())
    {
        return NSLOCTEXT("FaceTouchCorrection", "UnassignedTouch", "Face Touch");
    }
    if (interactionPointIds.Num() == 1)
    {
        return FText::Format(
            NSLOCTEXT("FaceTouchCorrection", "SingleTouch", "{0} → Face"),
            FText::FromName(interactionPointIds[0]));
    }
    return FText::Format(
        NSLOCTEXT("FaceTouchCorrection", "MultiTouch", "{0} points → Face"),
        FText::AsNumber(interactionPointIds.Num()));
}
