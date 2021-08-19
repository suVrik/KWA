#pragma once

#include "render/container/container_primitive.h"
#include "render/scene/camera.h"

#include <core/containers/unique_ptr.h>
#include <core/containers/vector.h>

namespace kw {

class aabbox;
class AccelerationStructure;
class AnimationPlayer;
class frustum;
class GeometryPrimitive;
class LightPrimitive;
class ParticleSystemPlayer;
class ParticleSystemPrimitive;

struct SceneDescriptor {
    AnimationPlayer* animation_player;
    ParticleSystemPlayer* particle_system_player;
    AccelerationStructure* geometry_acceleration_structure;
    AccelerationStructure* light_acceleration_structure;
    AccelerationStructure* particle_system_acceleration_structure;
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class Scene : public ContainerPrimitive {
public:
    explicit Scene(const SceneDescriptor& scene_descriptor);

    Vector<GeometryPrimitive*> query_geometry(const aabbox& bounds) const;
    Vector<GeometryPrimitive*> query_geometry(const frustum& frustum) const;
    
    Vector<LightPrimitive*> query_lights(const aabbox& bounds) const;
    Vector<LightPrimitive*> query_lights(const frustum& frustum) const;
    
    Vector<ParticleSystemPrimitive*> query_particle_systems(const aabbox& bounds) const;
    Vector<ParticleSystemPrimitive*> query_particle_systems(const frustum& frustum) const;

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

    AnimationPlayer& m_animation_player;
    ParticleSystemPlayer& m_particle_system_player;
    AccelerationStructure& m_geometry_acceleration_structure;
    AccelerationStructure& m_light_acceleration_structure;
    AccelerationStructure& m_particle_system_acceleration_structure;
    MemoryResource& m_transient_memory_resource;

    Camera m_camera;
    Camera m_occlusion_camera;
    bool m_is_occlusion_camera_used;
};

} // namespace kw
