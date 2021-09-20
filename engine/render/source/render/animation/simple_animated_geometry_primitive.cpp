#include "render/animation/simple_animated_geometry_primitive.h"
#include "render/animation/animation.h"
#include "render/animation/animation_manager.h"
#include "render/geometry/geometry.h"
#include "render/geometry/geometry_manager.h"
#include "render/material/material_manager.h"
#include "render/scene/render_primitive_reflection.h"

#include <core/debug/assert.h>
#include <core/io/markdown.h>
#include <core/io/markdown_utils.h>

namespace kw {

UniquePtr<Primitive> SimpleAnimatedGeometryPrimitive::create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node) {
    RenderPrimitiveReflection& render_reflection = dynamic_cast<RenderPrimitiveReflection&>(reflection);

    StringNode& animation_node = node["animation"].as<StringNode>();
    StringNode& geometry_node = node["geometry"].as<StringNode>();
    StringNode& material_node = node["material"].as<StringNode>();
    StringNode& shadow_material_node = node["shadow_material"].as<StringNode>();

    SharedPtr<Animation> animation = render_reflection.animation_manager.load(animation_node.get_value().c_str());
    SharedPtr<Geometry> geometry = render_reflection.geometry_manager.load(geometry_node.get_value().c_str());
    SharedPtr<Material> material = render_reflection.material_manager.load(material_node.get_value().c_str());
    SharedPtr<Material> shadow_material = render_reflection.material_manager.load(shadow_material_node.get_value().c_str());
    transform local_transform = MarkdownUtils::transform_from_markdown(node["local_transform"]);

    return static_pointer_cast<Primitive>(allocate_unique<SimpleAnimatedGeometryPrimitive>(
        reflection.memory_resource, reflection.memory_resource, animation, geometry, material, shadow_material, local_transform
    ));
}

SimpleAnimatedGeometryPrimitive::SimpleAnimatedGeometryPrimitive(MemoryResource& memory_resource, SharedPtr<Animation> animation,
                                                                 SharedPtr<Geometry> geometry, SharedPtr<Material> material,
                                                                 SharedPtr<Material> shadow_material, const transform& local_transform)
    : AnimatedGeometryPrimitive(memory_resource, geometry, material, shadow_material, local_transform)
    , m_animation(std::move(animation))
    , m_animation_time(0.f)
{
}

const SharedPtr<Animation>& SimpleAnimatedGeometryPrimitive::get_animation() const {
    return m_animation;
}

void SimpleAnimatedGeometryPrimitive::set_animation(SharedPtr<Animation> animation) {
    m_animation = std::move(animation);
}

float SimpleAnimatedGeometryPrimitive::get_animation_time() const {
    return m_animation_time;
}

void SimpleAnimatedGeometryPrimitive::set_animation_time(float value) {
    m_animation_time = value;
}

UniquePtr<Primitive> SimpleAnimatedGeometryPrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<SimpleAnimatedGeometryPrimitive>(memory_resource, *this));
}

void SimpleAnimatedGeometryPrimitive::update_animation(MemoryResource& transient_memory_resource, float elapsed_time) {
    const SharedPtr<Geometry>& geometry = get_geometry();
    const SharedPtr<Animation>& animation = get_animation();

    if (geometry && geometry->is_loaded() && animation && animation->is_loaded()) {
        SkeletonPose& skeleton_pose = get_skeleton_pose();

        KW_ASSERT(
            skeleton_pose.get_joint_count() == animation->get_joint_count(),
            "Mismatching animation and skeleton."
        );

        float old_time = get_animation_time();
        float new_time = old_time + elapsed_time * get_animation_speed();
        set_animation_time(new_time);

        for (uint32_t i = 0; i < animation->get_joint_count(); i++) {
            skeleton_pose.set_joint_space_transform(i, animation->get_joint_transform(i, new_time));
        }

        skeleton_pose.build_model_space_matrices(*geometry->get_skeleton());
        skeleton_pose.apply_inverse_bind_matrices(*geometry->get_skeleton());
    }
}

} // namespace kw
