#include <atomic>
#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libSceCamera/include/Camera.hpp"

namespace {

constexpr std::int32_t SCE_USER_SERVICE_USER_ID_SYSTEM = 0xFF;

struct CameraStartParameter {
    std::uint32_t sizeThis;
    std::uint32_t formatLevel[2];
    void* startOption;
};

std::atomic<std::int32_t> openHandles{0};

int CheckStart(std::int32_t handle, const CameraStartParameter* param) {
    if (handle < 1 || !param || param->sizeThis != sizeof(CameraStartParameter)) return SCE_CAMERA_ERROR_PARAM;
    if (!Camera::IsOpen()) return SCE_CAMERA_ERROR_NOT_OPEN;
    return 0;
}

}

bool Camera::IsOpen() {
    return openHandles.load() != 0;
}

extern "C" {

int APS5_VABI sceCameraOpen(std::int32_t userId, std::int32_t type, std::int32_t index, void*) {
    if (userId != SCE_USER_SERVICE_USER_ID_SYSTEM || type != 0 || index != 0) return SCE_CAMERA_ERROR_PARAM;
    return ++openHandles;
}

int APS5_VABI sceCameraClose(std::int32_t handle) {
    if (handle < 1) return SCE_CAMERA_ERROR_PARAM;
    std::int32_t open = openHandles.load();
    do {
        if (open == 0) return SCE_CAMERA_ERROR_NOT_OPEN;
    } while (!openHandles.compare_exchange_weak(open, open - 1));
    return 0;
}

int APS5_VABI sceCameraCloseByHandle(std::int32_t handle) {
    return sceCameraClose(handle);
}

int APS5_VABI sceCameraStart(std::int32_t handle, const CameraStartParameter* param) {
    if (const int error = CheckStart(handle, param)) return error;
    if (param->formatLevel[0] > 0xF || param->formatLevel[1] > 0xF || (param->formatLevel[0] | param->formatLevel[1]) == 0)
        return SCE_CAMERA_ERROR_FORMAT_UNKNOWN;
    return SCE_CAMERA_ERROR_NOT_CONNECTED;
}

int APS5_VABI sceCameraStartByHandle(std::int32_t handle, const CameraStartParameter* param) {
    return CheckStart(handle, param);
}

int APS5_VABI sceCameraStop(std::int32_t handle) {
    if (handle < 1) return SCE_CAMERA_ERROR_PARAM;
    if (!Camera::IsOpen()) return SCE_CAMERA_ERROR_NOT_OPEN;
    return 0;
}

int APS5_VABI sceCameraStopByHandle(std::int32_t handle) {
    return sceCameraStop(handle);
}

}
