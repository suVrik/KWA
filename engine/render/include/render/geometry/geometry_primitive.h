#pragma once

#include "render/acceleration_structure/acceleration_structure_primitive.h"

#include <core/containers/shared_ptr.h>

namespace kw {

class Geometry;
class Material;

class GeometryPrimitive : public AccelerationStructurePrimitive {
public:
    explicit GeometryPrimitive(SharedPtr<Geometry> geometry = nullptr, SharedPtr<Material> material = nullptr, const transform& local_transform = transform());
    ~GeometryPrimitive() override;

    const SharedPtr<Geometry>& get_geometry() const;
    void set_geometry(SharedPtr<Geometry> geometry);

    const SharedPtr<Material>& get_material() const;
    void set_material(SharedPtr<Material> geometry);

protected:
    // TODO: Update bounds in `global_transform_updated`.

private:
    SharedPtr<Geometry> m_geometry;
    SharedPtr<Material> m_material;
};

} // namespace kw
