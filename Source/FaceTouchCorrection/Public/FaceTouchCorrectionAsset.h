#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FaceTouchCorrectionTypes.h"
#include "FaceTouchCorrectionAsset.generated.h"

class UAnimSequenceBase;

UCLASS(BlueprintType)
class FACETOUCHCORRECTION_API UFaceTouchCorrectionAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    TObjectPtr<UAnimSequenceBase> sourceAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Face Touch Correction")
    TArray<FFaceTouchCorrectionBlock> corrections;
};
