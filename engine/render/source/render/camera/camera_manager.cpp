#include "render/camera/camera_manager.h"

namespace kw {

CameraManager::CameraManager()
    : m_is_occlusion_camera_used(false)
{
}

const Camera& CameraManager::get_camera() const {
    return m_camera;
}

Camera& CameraManager::get_camera() {
    return m_camera;
}

const Camera& CameraManager::get_occlusion_camera() const {
    if (m_is_occlusion_camera_used) {
        return m_occlusion_camera;
    } else {
        return m_camera;
    }
}

Camera& CameraManager::get_occlusion_camera() {
    if (m_is_occlusion_camera_used) {
        return m_occlusion_camera;
    } else {
        return m_camera;
    }
}

bool CameraManager::is_occlusion_camera_used() const {
    return m_is_occlusion_camera_used;
}

void CameraManager::toggle_occlusion_camera_used(bool value) {
    m_is_occlusion_camera_used = value;
}

} // namespace kw
