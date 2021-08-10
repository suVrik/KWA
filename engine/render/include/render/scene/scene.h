#pragma once

#include "render/container/container_primitive.h"
#include "render/scene/camera.h"

#include <core/containers/unique_ptr.h>
#include <core/containers/vector.h>

namespace kw {

class aabbox;
class AccelerationStructure;
class frustum;
class GeometryPrimitive;
class LightPrimitive;

class Scene : public ContainerPrimitive {
public:
    Scene(MemoryResource& persistent_memory_resource, MemoryResource& transient_memory_resource);
    ~Scene() override;

    Vector<GeometryPrimitive*> query_geometry(const aabbox& bounds);
    Vector<GeometryPrimitive*> query_geometry(const frustum& frustum);
    
    Vector<LightPrimitive*> query_lights(const aabbox& bounds);
    Vector<LightPrimitive*> query_lights(const frustum& frustum);

    const Camera& get_camera() const;
    Camera& get_camera();

    const Camera& get_occlusion_camera() const;
    Camera& get_occlusion_camera();

    bool is_occlusion_camera_used() const;
    void toggle_occlusion_camera_used(bool value);

protected:
    void child_added(Primitive& primitive) override;
    void child_removed(Primitive& primitive) override;

private:
    void add_container_primitive(ContainerPrimitive& container_primitive);
    void remove_container_primitive(ContainerPrimitive& container_primitive);

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UniquePtr<AccelerationStructure> m_geometry_acceleration_structure;
    UniquePtr<AccelerationStructure> m_light_acceleration_structure;

    Camera m_camera;
    Camera m_occlusion_camera;
    bool m_is_occlusion_camera_used;
};

} // namespace kw