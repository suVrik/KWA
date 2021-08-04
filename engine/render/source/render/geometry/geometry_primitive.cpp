#include "render/geometry/geometry_primitive.h"
#include "render/container/container_primitive.h"
#include "render/geometry/geometry.h"
#include "render/geometry/skeleton.h"

namespace kw {

GeometryPrimitive::GeometryPrimitive(SharedPtr<Geometry> geometry, SharedPtr<Material> material, const transform& local_transform)
    : AccelerationStructurePrimitive(local_transform)
    , m_geometry(std::move(geometry))
    , m_material(std::move(material))
{
}

GeometryPrimitive::~GeometryPrimitive() {
    // Scene must know whether the removed object is geometry, light or container to properly clean up acceleration
    // structures. If `remove_child` is called from `Primitive`, there's no way to know whether this was geometry,
    // light or container anymore.
    if (m_parent != nullptr) {
        m_parent->remove_child(*this);
    }
}

const SharedPtr<Geometry>& GeometryPrimitive::get_geometry() const {
    return m_geometry;
}

void GeometryPrimitive::set_geometry(SharedPtr<Geometry> geometry) {
    m_geometry = std::move(geometry);
}

const SharedPtr<Material>& GeometryPrimitive::get_material() const {
    return m_material;
}

void GeometryPrimitive::set_material(SharedPtr<Material> material) {
    m_material = std::move(material);
}

Vector<float4x4> GeometryPrimitive::get_model_space_joint_matrices(MemoryResource& memory_resource) {
    const Skeleton* skeleton = m_geometry->get_skeleton();
    if (skeleton != nullptr) {
        Vector<float4x4> result(skeleton->get_joint_count(), memory_resource);

        for (uint32_t i = 0; i < skeleton->get_joint_count(); i++) {
            uint32_t parent_joint_index = skeleton->get_parent_joint(i);
            if (parent_joint_index != UINT32_MAX) {
                result[i] = skeleton->get_bind_matrix(i) * result[parent_joint_index];
            } else {
                result[i] = skeleton->get_bind_matrix(i);
            }
        }

        for (uint32_t i = 0; i < skeleton->get_joint_count(); i++) {
            result[i] = skeleton->get_inverse_bind_matrix(i) * result[i];
        }

        return result;
    }

    return Vector<float4x4>(memory_resource);
}

} // namespace kw
