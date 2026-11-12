#include "FaceTouchCorrectionEditorModule.h"

#include "Components/SkeletalMeshComponent.h"
#include "EdMode.h"
#include "EditorModeManager.h"
#include "EditorModeRegistry.h"
#include "InputCoreTypes.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalRenderPublic.h"
#include "UnrealClient.h"

namespace
{
const FEditorModeID faceTouchCorrectionModeId = TEXT("EM_FaceTouchCorrection");

bool tracePosedSkeletalMesh(
    USkeletalMeshComponent& component,
    const FVector& traceStart,
    const FVector& traceEnd,
    FHitResult& outHit)
{
    const FSkeletalMeshRenderData* renderData = component.GetSkeletalMeshRenderData();
    if (renderData == nullptr || renderData->LODRenderData.IsEmpty())
    {
        return false;
    }

    const FSkeletalMeshLODRenderData& lodData = renderData->LODRenderData[0];
    TArray<uint32> indices;
    lodData.MultiSizeIndexContainer.GetIndexBuffer(indices);
    TArray<FFinalSkinVertex> vertices;
    component.GetCPUSkinnedVertices(vertices, 0);
    if (indices.IsEmpty() || vertices.IsEmpty())
    {
        return false;
    }

    const FTransform componentTransform = component.GetComponentTransform();
    double closestDistanceSquared = TNumericLimits<double>::Max();
    int32 closestTriangle = INDEX_NONE;
    FVector closestPoint = FVector::ZeroVector;
    FVector closestNormal = FVector::ZeroVector;
    for (int32 index = 0; index + 2 < indices.Num(); index += 3)
    {
        const uint32 indexA = indices[index];
        const uint32 indexB = indices[index + 1];
        const uint32 indexC = indices[index + 2];
        if (!vertices.IsValidIndex(indexA) ||
            !vertices.IsValidIndex(indexB) ||
            !vertices.IsValidIndex(indexC))
        {
            continue;
        }

        const FVector a = componentTransform.TransformPosition(
            FVector(vertices[indexA].Position));
        const FVector b = componentTransform.TransformPosition(
            FVector(vertices[indexB].Position));
        const FVector c = componentTransform.TransformPosition(
            FVector(vertices[indexC].Position));
        FVector intersection;
        FVector normal;
        if (!FMath::SegmentTriangleIntersection(
                traceStart,
                traceEnd,
                a,
                b,
                c,
                intersection,
                normal))
        {
            continue;
        }

        const double distanceSquared = FVector::DistSquared(traceStart, intersection);
        if (distanceSquared < closestDistanceSquared)
        {
            closestDistanceSquared = distanceSquared;
            closestTriangle = index / 3;
            closestPoint = intersection;
            closestNormal = normal;
        }
    }

    if (closestTriangle == INDEX_NONE)
    {
        return false;
    }

    outHit.FaceIndex = closestTriangle;
    outHit.ImpactPoint = closestPoint;
    outHit.ImpactNormal = closestNormal.GetSafeNormal();
    outHit.Distance = FMath::Sqrt(closestDistanceSquared);
    return true;
}

class FFaceTouchCorrectionEdMode : public FEdMode
{
public:
    virtual bool HandleClick(
        FEditorViewportClient* viewportClient,
        HHitProxy* hitProxy,
        const FViewportClick& click) override
    {
        return static_cast<FFaceTouchCorrectionEditorModule&>(
            IFaceTouchCorrectionEditorModule::get()).handleViewportClick(click);
    }
};
}

void FFaceTouchCorrectionEditorModule::StartupModule()
{
    FEditorModeRegistry::Get().RegisterMode<FFaceTouchCorrectionEdMode>(
        faceTouchCorrectionModeId,
        NSLOCTEXT("FaceTouchCorrection", "EditorModeName", "Face Touch Correction"),
        FSlateIcon(),
        false);
}

void FFaceTouchCorrectionEditorModule::ShutdownModule()
{
    targetActor.Reset();
    onSurfaceClicked.Unbind();
    FEditorModeRegistry::Get().UnregisterMode(faceTouchCorrectionModeId);
}

void FFaceTouchCorrectionEditorModule::beginFacePicking(
    AActor* newTargetActor,
    FOnFaceTouchSurfaceClicked newOnSurfaceClicked)
{
    targetActor = newTargetActor;
    onSurfaceClicked = MoveTemp(newOnSurfaceClicked);
    if (newTargetActor != nullptr)
    {
        GLevelEditorModeTools().ActivateMode(faceTouchCorrectionModeId);
    }
}

void FFaceTouchCorrectionEditorModule::endFacePicking()
{
    if (!IsEngineExitRequested() &&
        GEditor != nullptr &&
        GLevelEditorModeTools().IsModeActive(faceTouchCorrectionModeId))
    {
        GLevelEditorModeTools().DeactivateMode(faceTouchCorrectionModeId);
    }
    targetActor.Reset();
    onSurfaceClicked.Unbind();
}

bool FFaceTouchCorrectionEditorModule::isFacePicking() const
{
    return targetActor.IsValid() &&
        !IsEngineExitRequested() &&
        GEditor != nullptr &&
        GLevelEditorModeTools().IsModeActive(faceTouchCorrectionModeId);
}

bool FFaceTouchCorrectionEditorModule::handleViewportClick(
    const FViewportClick& viewportClick)
{
    AActor* actor = targetActor.Get();
    if (actor == nullptr || viewportClick.GetKey() != EKeys::LeftMouseButton)
    {
        return false;
    }

    const FVector traceStart = viewportClick.GetOrigin();
    const FVector traceEnd = traceStart + viewportClick.GetDirection() * 1000000.0;
    FCollisionQueryParams queryParams(SCENE_QUERY_STAT(FaceTouchCorrection), true);
    queryParams.bReturnFaceIndex = true;

    FHitResult bestHit;
    USkeletalMeshComponent* bestComponent = nullptr;
    TArray<USkeletalMeshComponent*> skeletalMeshComponents;
    actor->GetComponents(skeletalMeshComponents);
    for (USkeletalMeshComponent* component : skeletalMeshComponents)
    {
        if (component == nullptr || !component->IsVisible())
        {
            continue;
        }
        FHitResult candidate;
        if ((component->LineTraceComponent(
                candidate,
                traceStart,
                traceEnd,
                queryParams) ||
             tracePosedSkeletalMesh(
                 *component,
                 traceStart,
                 traceEnd,
                 candidate)) &&
            (bestComponent == nullptr || candidate.Distance < bestHit.Distance))
        {
            bestHit = candidate;
            bestComponent = component;
        }
    }

    if (bestComponent == nullptr)
    {
        return false;
    }

    FFaceTouchSurfaceLocation location;
    location.skeletalMeshComponentName = bestComponent->GetFName();
    location.triangleIndex = bestHit.FaceIndex;
    location.componentLocalPosition = bestComponent->GetComponentTransform()
        .InverseTransformPosition(bestHit.ImpactPoint);
    location.componentLocalNormal = bestComponent->GetComponentTransform()
        .InverseTransformVectorNoScale(bestHit.ImpactNormal)
        .GetSafeNormal();
    const FVector componentLocalTraceDirection =
        bestComponent->GetComponentTransform()
            .InverseTransformVectorNoScale(viewportClick.GetDirection())
            .GetSafeNormal();
    if (FVector::DotProduct(
            location.componentLocalNormal,
            componentLocalTraceDirection) > 0.0)
    {
        location.componentLocalNormal *= -1.0;
    }
    onSurfaceClicked.ExecuteIfBound(location);
    return true;
}

IMPLEMENT_MODULE(FFaceTouchCorrectionEditorModule, FaceTouchCorrectionEditor)
