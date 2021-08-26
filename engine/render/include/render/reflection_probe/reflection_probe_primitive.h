#pragma once

#include "render/acceleration_structure/acceleration_structure_primitive.h"

#include <core/containers/shared_ptr.h>

namespace kw {

struct PrimitiveReflectionDescriptor;
class ReflectionProbeManager;
class Texture;

class ReflectionProbePrimitive : public AccelerationStructurePrimitive {
public:
    static UniquePtr<Primitive> create_from_markdown(const PrimitiveReflectionDescriptor& primitive_reflection_descriptor);

    explicit ReflectionProbePrimitive(SharedPtr<Texture*> irradiance_map = nullptr,
                                      SharedPtr<Texture*> prefiltered_environment_map = nullptr,
                                      float falloff_radius = 1.f,
                                      const aabbox& parallax_box = aabbox(),
                                      const transform& local_transform = transform());

    // Reflection probe manager is set from ReflectionProbeManager's `add` method.
    ReflectionProbeManager* get_reflection_probe_manager() const;

    const SharedPtr<Texture*>& get_irradiance_map() const;
    void set_irradiance_map(SharedPtr<Texture*> texture);

    const SharedPtr<Texture*>& get_prefiltered_environment_map() const;
    void set_prefiltered_environment_map(SharedPtr<Texture*> texture);

    float get_falloff_radius() const;
    void set_falloff_radius(float value);

    // Parallax box is defined in world space, not local space.
    const aabbox& get_parallax_box() const;
    void set_parallax_box(const aabbox& value);

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void global_transform_updated() override;

private:
    ReflectionProbeManager* m_reflection_probe_manager;

    SharedPtr<Texture*> m_irradiance_map;
    SharedPtr<Texture*> m_prefiltered_environment_map;

    float m_falloff_radius;
    aabbox m_parallax_box;

    // Friendship is needed to access `m_reflection_probe_manager`.
    friend class ReflectionProbeManager;
};

} // namespace kw
