#pragma once

#include "CoreMinimal.h"
#include "FaceTouchCorrectionTypes.generated.h"

UENUM(BlueprintType)
enum class EFaceTouchHandSide : uint8
{
    Left,
    Right
};

USTRUCT(BlueprintType)
struct FACETOUCHCORRECTION_API FFaceTouchSurfaceLocation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    FName skeletalMeshComponentName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    int32 lodIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    int32 triangleIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    FVector barycentricCoordinates = FVector(1.0, 0.0, 0.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    float normalOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    FVector componentLocalPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    FVector componentLocalNormal = FVector::ForwardVector;

    bool isValid() const;
};

USTRUCT(BlueprintType)
struct FACETOUCHCORRECTION_API FFaceTouchCorrectionBlock
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Face Touch Correction")
    FGuid correctionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    EFaceTouchHandSide handSide = EFaceTouchHandSide::Right;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    TArray<FName> interactionPointIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    int32 solveFrame = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    int32 approachZeroFrame = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    int32 touchStartFrame = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    int32 touchEndFrame = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    int32 recoveryZeroFrame = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    FFaceTouchSurfaceLocation surfaceLocation;

    FFaceTouchCorrectionBlock();

    bool isValid() const;
    double evaluateWeight(double frame) const;
    void shiftBy(int32 frameDelta, int32 lastFrame);
    FText getDisplayName() const;
};
