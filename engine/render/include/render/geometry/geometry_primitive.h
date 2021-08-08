#pragma once

#include "render/acceleration_structure/acceleration_structure_primitive.h"
#include "render/geometry/geometry_listener.h"

#include <core/containers/shared_ptr.h>
#include <core/containers/vector.h>

namespace kw {

class Geometry;
class Material;

class GeometryPrimitive : public AccelerationStructurePrimitive, public GeometryListener {
public:
    explicit GeometryPrimitive(SharedPtr<Geometry> geometry = nullptr,
                               SharedPtr<Material> material = nullptr,
                               const transform& local_transform = transform());
    GeometryPrimitive(const GeometryPrimitive& other);
    GeometryPrimitive(GeometryPrimitive&& other);
    ~GeometryPrimitive() override;
    GeometryPrimitive& operator=(const GeometryPrimitive& other);
    GeometryPrimitive& operator=(GeometryPrimitive&& other);

    const SharedPtr<Geometry>& get_geometry() const;
    void set_geometry(SharedPtr<Geometry> geometry);

    const SharedPtr<Material>& get_material() const;
    void set_material(SharedPtr<Material> geometry);

    // Returns joint transformation matrcies in model space. Returns an empty array if this geometry is not skinned.
    // Returns default bind pose if this geometry doesn't have a custom pose (i.e. not an `AnimatedGeometryPrimitive`).
    virtual Vector<float4x4> get_model_space_joint_matrices(MemoryResource& memory_resource);

protected:
    void global_transform_updated() override;
    void geometry_loaded() override;

private:
    SharedPtr<Geometry> m_geometry;
    SharedPtr<Material> m_material;
};

} // namespace kw
