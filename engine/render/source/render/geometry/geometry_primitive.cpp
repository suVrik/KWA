#include "render/geometry/geometry_primitive.h"

namespace kw {

GeometryPrimitive::GeometryPrimitive(SharedPtr<Geometry> geometry, SharedPtr<Material> material)
    : m_geometry(std::move(geometry))
    , m_material(std::move(material))
{
}

GeometryPrimitive::~GeometryPrimitive() = default;

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

} // namespace kw
