#include "render/scene/render_primitive_reflection.h"
#include "render/animation/animated_geometry_primitive.h"
#include "render/geometry/geometry_primitive.h"
#include "render/light/point_light_primitive.h"
#include "render/particles/particle_system_primitive.h"
#include "render/reflection_probe/reflection_probe_primitive.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown.h>
#include <core/prefab/prefab_primitive.h>

namespace kw {

#define KW_NAME_AND_CALLBACK(Type) String(#Type, MallocMemoryResource::instance()), &Type::create_from_markdown

RenderPrimitiveReflection::RenderPrimitiveReflection(const RenderPrimitiveReflectionDescriptor& descriptor)
    : PrimitiveReflection(PrimitiveReflectionDescriptor{ descriptor.prefab_manager, descriptor.memory_resource })
    , texture_manager(*descriptor.texture_manager)
    , geometry_manager(*descriptor.geometry_manager)
    , material_manager(*descriptor.material_manager)
    , animation_manager(*descriptor.animation_manager)
    , particle_system_manager(*descriptor.particle_system_manager)
{
    KW_ASSERT(descriptor.texture_manager != nullptr, "Invalid texture manager.");
    KW_ASSERT(descriptor.geometry_manager != nullptr, "Invalid geometry manager.");
    KW_ASSERT(descriptor.material_manager != nullptr, "Invalid material manager.");
    KW_ASSERT(descriptor.animation_manager != nullptr, "Invalid animation manager.");
    KW_ASSERT(descriptor.particle_system_manager != nullptr, "Invalid particle system manager.");

    m_primitives.emplace(KW_NAME_AND_CALLBACK(AnimatedGeometryPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(GeometryPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(ParticleSystemPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(PointLightPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(ReflectionProbePrimitive));
}

#undef KW_NAME_AND_CALLBACK

} // namespace kw
