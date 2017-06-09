#include "AirSim.h"
#include "PIPCamera.h"
#include "AirBlueprintLib.h"
#include "ImageUtils.h"
#include <string>

APIPCamera::APIPCamera()
{
    static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> scene_render_target_finder(TEXT("TextureRenderTarget2D'/AirSim/HUDAssets/FpvRenderTarget.FpvRenderTarget'"));
    scene_render_target_ = scene_render_target_finder.Object;

    static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> depth_render_target_finder(TEXT("TextureRenderTarget2D'/AirSim/HUDAssets/DepthRenderTarget.DepthRenderTarget'"));
    depth_render_target_ = depth_render_target_finder.Object;

    static ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D> seg_render_target_finder(TEXT("TextureRenderTarget2D'/AirSim/HUDAssets/SegmentationRenderTarget.SegmentationRenderTarget'"));
    seg_render_target_ = seg_render_target_finder.Object;
}

void APIPCamera::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    camera_ = UAirBlueprintLib::GetActorComponent<UCameraComponent>(this, TEXT("CameraComponent"));

    scene_capture_ = UAirBlueprintLib::GetActorComponent<USceneCaptureComponent2D>(this, TEXT("SceneCaptureComponent"));
    depth_capture_ = UAirBlueprintLib::GetActorComponent<USceneCaptureComponent2D>(this, TEXT("SceneCaptureComponent2"));
    seg_capture_ = UAirBlueprintLib::GetActorComponent<USceneCaptureComponent2D>(this, TEXT("SceneCaptureComponent3"));
}

void APIPCamera::setToMainView()
{
    camera_->Activate();
    APlayerController* controller = this->GetWorld()->GetFirstPlayerController();
    controller->SetViewTarget(this);
    deactivatePIP();

    camera_mode_ = EPIPCameraMode::PIP_CAMERA_MODE_MAIN;
}

void APIPCamera::setToPIPView()
{
    deactivateMain();

    activateCaptureComponent(EPIPCameraType::PIP_CAMERA_TYPE_SCENE);
    activateCaptureComponent(EPIPCameraType::PIP_CAMERA_TYPE_DEPTH);
    activateCaptureComponent(EPIPCameraType::PIP_CAMERA_TYPE_SEG);

    camera_mode_ = EPIPCameraMode::PIP_CAMERA_MODE_PIP;
}

void APIPCamera::disableAll()
{
    deactivateMain();
    deactivatePIP();

    camera_mode_ = EPIPCameraMode::PIP_CAMERA_MODE_NONE;
}

void APIPCamera::deactivateCaptureComponent(const EPIPCameraType type)
{
    USceneCaptureComponent2D* capture = getCaptureComponent(type, false);
    if (capture != nullptr) {
        capture->Deactivate();
        capture->TextureTarget = nullptr;
    }
}

void APIPCamera::activateCaptureComponent(const EPIPCameraType type)
{
    USceneCaptureComponent2D* capture = getCaptureComponent(type, true);
    if (capture != nullptr) {
        capture->TextureTarget = getTextureRenderTarget(type, false);
        capture->Activate();
    }
}

UTextureRenderTarget2D* APIPCamera::getTextureRenderTarget(const EPIPCameraType type, bool if_active)
{
    switch (type) {
    case EPIPCameraType::PIP_CAMERA_TYPE_SCENE:
        if (!if_active || (static_cast<uint8>(enabled_camera_types_) & static_cast<uint8>(EPIPCameraType::PIP_CAMERA_TYPE_SCENE)))
            return scene_render_target_;
        return nullptr;
    case EPIPCameraType::PIP_CAMERA_TYPE_DEPTH:
        if (!if_active || (static_cast<uint8>(enabled_camera_types_) & static_cast<uint8>(EPIPCameraType::PIP_CAMERA_TYPE_DEPTH)))
            return depth_render_target_;
        return nullptr;
    case EPIPCameraType::PIP_CAMERA_TYPE_SEG:
        if (!if_active || (static_cast<uint8>(enabled_camera_types_) & static_cast<uint8>(EPIPCameraType::PIP_CAMERA_TYPE_SEG)))
            return seg_render_target_;
        return nullptr;
    case EPIPCameraType::PIP_CAMERA_TYPE_NONE:
        return nullptr;
    default:
        return nullptr;
        UAirBlueprintLib::LogMessageString("Cannot get render target because camera type is not recognized", std::to_string(static_cast<uint8>(type)), LogDebugLevel::Failure, 60);
    }

}

USceneCaptureComponent2D* APIPCamera::getCaptureComponent(const EPIPCameraType type, bool if_active)
{
    switch (type) {
    case EPIPCameraType::PIP_CAMERA_TYPE_SCENE:
        if (!if_active || (static_cast<uint8>(enabled_camera_types_) & static_cast<uint8>(EPIPCameraType::PIP_CAMERA_TYPE_SCENE)))
            return scene_capture_;
        return nullptr;
    case EPIPCameraType::PIP_CAMERA_TYPE_DEPTH:
        if (!if_active || (static_cast<uint8>(enabled_camera_types_) & static_cast<uint8>(EPIPCameraType::PIP_CAMERA_TYPE_DEPTH)))
            return depth_capture_;
        return nullptr;
    case EPIPCameraType::PIP_CAMERA_TYPE_SEG:
        if (!if_active || (static_cast<uint8>(enabled_camera_types_) & static_cast<uint8>(EPIPCameraType::PIP_CAMERA_TYPE_SEG)))
            return seg_capture_;
        return nullptr;
    case EPIPCameraType::PIP_CAMERA_TYPE_NONE:
        return nullptr;
    default:
        return nullptr;
        UAirBlueprintLib::LogMessageString("Cannot get capture component because camera type is not recognized", std::to_string(static_cast<uint8>(type)), LogDebugLevel::Failure, 60);
    }

}


void APIPCamera::deactivatePIP()
{
    deactivateCaptureComponent(EPIPCameraType::PIP_CAMERA_TYPE_SCENE);
    deactivateCaptureComponent(EPIPCameraType::PIP_CAMERA_TYPE_DEPTH);
    deactivateCaptureComponent(EPIPCameraType::PIP_CAMERA_TYPE_SEG);
}


void APIPCamera::deactivateMain()
{
    camera_->Deactivate();
}

void APIPCamera::refreshCurrentMode()
{
    switch (camera_mode_) {
    case EPIPCameraMode::PIP_CAMERA_MODE_NONE:
        disableAll();
        break;
    case EPIPCameraMode::PIP_CAMERA_MODE_MAIN:
        setToMainView();
        break;
    case EPIPCameraMode::PIP_CAMERA_MODE_PIP:
        setToPIPView();
        break;
    default:
        UAirBlueprintLib::LogMessageString("Cannot refresh current mode because its value is not recognized", std::to_string(static_cast<uint8>(camera_mode_)), LogDebugLevel::Failure, 60);
    }
}

EPIPCameraMode APIPCamera::getCameraMode()
{
    return camera_mode_;
}

EPIPCameraType APIPCamera::toggleEnableCameraTypes(EPIPCameraType types)
{
    enabled_camera_types_ = enabled_camera_types_ ^ types;
    deactivatePIP();
    refreshCurrentMode();

    return enabled_camera_types_ & types;
}

EPIPCameraType APIPCamera::getEnableCameraTypes()
{
    return enabled_camera_types_;
}

void APIPCamera::setEnableCameraTypes(EPIPCameraType types)
{
    enabled_camera_types_ = types;
}