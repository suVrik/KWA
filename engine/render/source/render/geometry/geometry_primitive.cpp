#include "render/geometry/geometry_primitive.h"
#include "render/geometry/geometry.h"
#include "render/geometry/skeleton.h"

#include <core/debug/assert.h>

#include <atomic>

namespace kw {

// Declared in `render/acceleration_structure/acceleration_structure_primitive.cpp`.
extern std::atomic_uint64_t acceleration_structure_counter;

GeometryPrimitive::GeometryPrimitive(SharedPtr<Geometry> geometry, SharedPtr<Material> material, const transform& local_transform)
    : AccelerationStructurePrimitive(local_transform)
    , m_geometry(std::move(geometry))
    , m_material(std::move(material))
{
    // If geometry is not loaded yet, it will end up in center acceleration structure's node.

    if (m_geometry) {
        // If geometry is already loaded, `geometry_loaded` will be called immediately.
        m_geometry->subscribe(*this);
    }
}

GeometryPrimitive::GeometryPrimitive(const GeometryPrimitive& other)
    : AccelerationStructurePrimitive(other)
    , m_geometry(other.m_geometry)
    , m_material(other.m_material)
{
    if (m_geometry) {
        // If geometry is already loaded, `geometry_loaded` will be called immediately.
        m_geometry->subscribe(*this);
    }
}

GeometryPrimitive::GeometryPrimitive(GeometryPrimitive&& other)
    : AccelerationStructurePrimitive(std::move(other))
    , m_material(std::move(other.m_material))
{
    if (other.m_geometry) {
        // No effect if `geometry_loaded` for this primitive & geometry was already called.
        other.m_geometry->unsubscribe(other);

        m_geometry = std::move(other.m_geometry);

        // If geometry is already loaded, `geometry_loaded` will be called immediately.
        m_geometry->subscribe(*this);
    }
}

GeometryPrimitive::~GeometryPrimitive() {
    if (m_geometry) {
        // No effect if `geometry_loaded` for this primitive & geometry was already called.
        m_geometry->unsubscribe(*this);
    }
}

GeometryPrimitive& GeometryPrimitive::operator=(const GeometryPrimitive& other) {
    AccelerationStructurePrimitive::operator=(other);

    if (m_geometry) {
        // No effect if `geometry_loaded` for this primitive & geometry was already called.
        m_geometry->unsubscribe(*this);
    }

    m_geometry = other.m_geometry;
    m_material = other.m_material;

    if (m_geometry) {
        // If geometry is already loaded, `geometry_loaded` will be called immediately.
        m_geometry->subscribe(*this);
    }

    return *this;
}

GeometryPrimitive& GeometryPrimitive::operator=(GeometryPrimitive&& other) {
    AccelerationStructurePrimitive::operator=(std::move(other));

    m_material = std::move(other.m_material);

    if (m_geometry) {
        // No effect if `geometry_loaded` for this primitive & geometry was already called.
        m_geometry->unsubscribe(*this);
    }

    if (other.m_geometry) {
        // No effect if `geometry_loaded` for this primitive & geometry was already called.
        other.m_geometry->unsubscribe(other);

        m_geometry = std::move(other.m_geometry);

        // If geometry is already loaded, `geometry_loaded` will be called immediately.
        m_geometry->subscribe(*this);
    }

    return *this;
}

const SharedPtr<Geometry>& GeometryPrimitive::get_geometry() const {
    return m_geometry;
}

void GeometryPrimitive::set_geometry(SharedPtr<Geometry> geometry) {
    if (m_geometry != geometry) {
        m_counter = ++acceleration_structure_counter;

        if (m_geometry) {
            // No effect if `geometry_loaded` for this primitive & geometry was already called.
            m_geometry->unsubscribe(*this);
        }

        m_geometry = std::move(geometry);

        if (m_geometry) {
            // If geometry is already loaded, `geometry_loaded` will be called immediately.
            m_geometry->subscribe(*this);
        }
    }
}

const SharedPtr<Material>& GeometryPrimitive::get_material() const {
    return m_material;
}

void GeometryPrimitive::set_material(SharedPtr<Material> material) {
    if (m_material != material) {
        m_counter = ++acceleration_structure_counter;

        m_material = std::move(material);
    }
}

Vector<float4x4> GeometryPrimitive::get_model_space_joint_matrices(MemoryResource& memory_resource) {
    if (m_geometry && m_geometry->is_loaded()) {
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
    }

    return Vector<float4x4>(memory_resource);
}

void GeometryPrimitive::global_transform_updated() {
    if (m_geometry && m_geometry->is_loaded()) {
        m_bounds = m_geometry->get_bounds() * get_global_transform();
    }

    AccelerationStructurePrimitive::global_transform_updated();
}

void GeometryPrimitive::geometry_loaded() {
    // This method must be called from geometry manager, which knows for sure that geometry is loaded.
    KW_ASSERT(m_geometry && m_geometry->is_loaded(), "Geometry must be loaded.");

    m_bounds = m_geometry->get_bounds() * get_global_transform();

    // Update acceleration structure's node.
    AccelerationStructurePrimitive::global_transform_updated();
}

} // namespace kw
