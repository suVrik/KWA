#include "physics/scene/capsule_controller_primitive.h"

#include <core/io/markdown.h>
#include <core/io/markdown_utils.h>
#include <core/scene/primitive_reflection.h>

#include <characterkinematic/PxCapsuleController.h>

namespace kw {

UniquePtr<Primitive> CapsuleControllerPrimitive::create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node) {
    float radius = node["radius"].as<NumberNode>().get_value();
    float height = node["height"].as<NumberNode>().get_value();
    float step_offset = node["step_offset"].as<NumberNode>().get_value();
    transform local_transform = MarkdownUtils::transform_from_markdown(node["local_transform"]);

    return static_pointer_cast<Primitive>(allocate_unique<CapsuleControllerPrimitive>(
        reflection.memory_resource, radius, height, step_offset, local_transform
    ));
}

CapsuleControllerPrimitive::CapsuleControllerPrimitive(float radius, float height, float step_offset,
                                                       const transform& local_transform)
    : ControllerPrimitive(step_offset, local_transform)
    , m_radius(radius)
    , m_height(height)
{
}

CapsuleControllerPrimitive::~CapsuleControllerPrimitive() = default;

float CapsuleControllerPrimitive::get_radius() const {
    return m_radius;
}

void CapsuleControllerPrimitive::set_radius(float value) {
    m_radius = value;

    if (m_controller) {
        static_cast<physx::PxCapsuleController*>(m_controller.get())->setRadius(m_radius);
    }
}

float CapsuleControllerPrimitive::get_height() const {
    return m_height;
}

void CapsuleControllerPrimitive::set_height(float value) {
    m_height = value;

    if (m_controller) {
        static_cast<physx::PxCapsuleController*>(m_controller.get())->setHeight(m_height);
    }
}

UniquePtr<Primitive> CapsuleControllerPrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<CapsuleControllerPrimitive>(memory_resource, *this));
}

} // namespace kw
