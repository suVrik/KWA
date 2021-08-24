#pragma once

#include "render/camera/camera.h"

namespace kw {

class CameraManager {
public:
    CameraManager();

    const Camera& get_camera() const;
    Camera& get_camera();

    const Camera& get_occlusion_camera() const;
    Camera& get_occlusion_camera();

    bool is_occlusion_camera_used() const;
    void toggle_occlusion_camera_used(bool value);

private:
    Camera m_camera;
    Camera m_occlusion_camera;
    bool m_is_occlusion_camera_used;
};

} // namespace kw
