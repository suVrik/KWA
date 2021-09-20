#include "render/scene/render_primitive_reflection.h"
#include "render/animation/simple_animated_geometry_primitive.h"
#include "render/geometry/geometry_primitive.h"
#include "render/light/point_light_primitive.h"
#include "render/motion/motion_geometry_primitive.h"
#include "render/particles/particle_system_primitive.h"
#include "render/reflection_probe/reflection_probe_primitive.h"

#include <core/debug/assert.h>

namespace kw {

#define KW_NAME_AND_CALLBACK(Type) String(#Type, MallocMemoryResource::instance()), &Type::create_from_markdown

RenderPrimitiveReflection::RenderPrimitiveReflection(const RenderPrimitiveReflectionDescriptor& descriptor)
    : PrimitiveReflection(PrimitiveReflectionDescriptor{ descriptor.prefab_manager, descriptor.memory_resource })
    , texture_manager(*descriptor.texture_manager)
    , geometry_manager(*descriptor.geometry_manager)
    , material_manager(*descriptor.material_manager)
    , animation_manager(*descriptor.animation_manager)
    , motion_graph_manager(*descriptor.motion_graph_manager)
    , particle_system_manager(*descriptor.particle_system_manager)
{
    KW_ASSERT(descriptor.texture_manager != nullptr, "Invalid texture manager.");
    KW_ASSERT(descriptor.geometry_manager != nullptr, "Invalid geometry manager.");
    KW_ASSERT(descriptor.material_manager != nullptr, "Invalid material manager.");
    KW_ASSERT(descriptor.animation_manager != nullptr, "Invalid animation manager.");
    KW_ASSERT(descriptor.motion_graph_manager != nullptr, "Invalid motion graph manager.");
    KW_ASSERT(descriptor.particle_system_manager != nullptr, "Invalid particle system manager.");

    m_primitives.emplace(KW_NAME_AND_CALLBACK(GeometryPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(MotionGeometryPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(ParticleSystemPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(PointLightPrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(ReflectionProbePrimitive));
    m_primitives.emplace(KW_NAME_AND_CALLBACK(SimpleAnimatedGeometryPrimitive));
}

#undef KW_NAME_AND_CALLBACK

} // namespace kw
