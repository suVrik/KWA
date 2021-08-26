#include "render/scene/primitive_reflection.h"
#include "render/animation/animated_geometry_primitive.h"
#include "render/container/container_primitive.h"
#include "render/geometry/geometry_primitive.h"
#include "render/light/point_light_primitive.h"
#include "render/particles/particle_system_primitive.h"
#include "render/reflection_probe/reflection_probe_primitive.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown.h>

namespace kw {

PrimitiveReflection& PrimitiveReflection::instance() {
    static PrimitiveReflection reflection;
    return reflection;
}

UniquePtr<Primitive> PrimitiveReflection::create_from_markdown(const PrimitiveReflectionDescriptor& descriptor) {
    KW_ASSERT(descriptor.primitive_node != nullptr);
    KW_ASSERT(descriptor.texture_manager != nullptr);
    KW_ASSERT(descriptor.geometry_manager != nullptr);
    KW_ASSERT(descriptor.material_manager != nullptr);
    KW_ASSERT(descriptor.animation_manager != nullptr);
    KW_ASSERT(descriptor.particle_system_manager != nullptr);
    KW_ASSERT(descriptor.container_manager != nullptr);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);

    const StringNode& type_name = (*descriptor.primitive_node)["type"].as<StringNode>();

    auto it = m_primitives.find(type_name.get_value());
    KW_ERROR(it != m_primitives.end(), "Invalid primitive type.");

    return it->second(descriptor);
}

#define KW_NAME_AND_CALLBACK(Type) String(#Type, MallocMemoryResource::instance()), &Type::create_from_markdown

PrimitiveReflection::PrimitiveReflection()
    : m_primitives(MallocMemoryResource::instance())
{
    m_primitives.emplace(KW_NAME_AND_CALLBACK(AnimatedGeometryPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(ContainerPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(GeometryPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(ParticleSystemPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(PointLightPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(ReflectionProbePrimitive));
}

#undef KW_NAME_AND_CALLBACK

} // namespace kw
