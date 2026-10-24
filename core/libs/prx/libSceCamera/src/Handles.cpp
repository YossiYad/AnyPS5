#include <atomic>
#include <cstdint>
#include "SceTypes.hpp"
#include "prx/libSceCamera/include/Camera.hpp"

namespace {

constexpr std::int32_t SCE_USER_SERVICE_USER_ID_SYSTEM = 0xFF;

std::atomic<std::int32_t> openHandles{0};

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

}
