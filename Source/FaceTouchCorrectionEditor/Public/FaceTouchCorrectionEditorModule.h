#pragma once

#include "FaceTouchCorrectionTypes.h"
#include "Modules/ModuleManager.h"

class AActor;
struct FViewportClick;

DECLARE_DELEGATE_OneParam(
    FOnFaceTouchSurfaceClicked,
    const FFaceTouchSurfaceLocation&)

class FACETOUCHCORRECTIONEDITOR_API IFaceTouchCorrectionEditorModule
    : public IModuleInterface
{
public:
    static IFaceTouchCorrectionEditorModule& get()
    {
        return FModuleManager::LoadModuleChecked<IFaceTouchCorrectionEditorModule>(
            TEXT("FaceTouchCorrectionEditor"));
    }

    virtual void beginFacePicking(
        AActor* targetActor,
        FOnFaceTouchSurfaceClicked onSurfaceClicked) = 0;
    virtual void endFacePicking() = 0;
    virtual bool isFacePicking() const = 0;
};

class FFaceTouchCorrectionEditorModule : public IFaceTouchCorrectionEditorModule
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    virtual void beginFacePicking(
        AActor* targetActor,
        FOnFaceTouchSurfaceClicked onSurfaceClicked) override;
    virtual void endFacePicking() override;
    virtual bool isFacePicking() const override;

    bool handleViewportClick(const FViewportClick& viewportClick);

private:
    TWeakObjectPtr<AActor> targetActor;
    FOnFaceTouchSurfaceClicked onSurfaceClicked;
};
