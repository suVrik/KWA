#include "render/animation/animated_geometry_primitive.h"
#include "render/animation/animation_player.h"
#include "render/geometry/geometry.h"
#include "render/geometry/skeleton.h"

#include <core/debug/assert.h>

#include <atomic>

namespace kw {

// Declared in `render/acceleration_structure/acceleration_structure_primitive.cpp`.
extern std::atomic_uint64_t acceleration_structure_counter;

AnimatedGeometryPrimitive::AnimatedGeometryPrimitive(MemoryResource& memory_resource, SharedPtr<Animation> animation,
                                                     SharedPtr<Geometry> geometry, SharedPtr<Material> material,
                                                     SharedPtr<Material> shadow_material, const transform& local_transform)
    : GeometryPrimitive(geometry, material, shadow_material, local_transform)
    , m_animation_player(nullptr)
    , m_skeleton_pose(memory_resource)
    , m_animation(std::move(animation))
    , m_animation_time(0.f)
    , m_animation_speed(1.f)
{
}

AnimatedGeometryPrimitive::AnimatedGeometryPrimitive(const AnimatedGeometryPrimitive& other)
    : GeometryPrimitive(other)
    , m_animation_player(nullptr)
    , m_skeleton_pose(other.m_skeleton_pose)
    , m_animation(other.m_animation)
    , m_animation_time(other.m_animation_time)
    , m_animation_speed(other.m_animation_speed)
{
    KW_ASSERT(
        other.m_animation_player == nullptr,
        "Copying animated geometry primitives with an animation player assigned is not allowed."
    );
}

AnimatedGeometryPrimitive::AnimatedGeometryPrimitive(AnimatedGeometryPrimitive&& other)
    : GeometryPrimitive(std::move(other))
    , m_animation_player(nullptr)
    , m_skeleton_pose(std::move(other.m_skeleton_pose))
    , m_animation(std::move(other.m_animation))
    , m_animation_time(other.m_animation_time)
    , m_animation_speed(other.m_animation_speed)
{
    KW_ASSERT(
        other.m_animation_player == nullptr,
        "Copying animated geometry primitives with an animation player assigned is not allowed."
    );
}

AnimatedGeometryPrimitive::~AnimatedGeometryPrimitive() {
    if (m_animation_player != nullptr) {
        m_animation_player->remove(*this);
    }
}

AnimatedGeometryPrimitive& AnimatedGeometryPrimitive::operator=(const AnimatedGeometryPrimitive& other) {
    GeometryPrimitive::operator=(other);

    if (m_animation_player != nullptr) {
        m_animation_player->remove(*this);
    }

    m_skeleton_pose = other.m_skeleton_pose;
    m_animation = other.m_animation;
    m_animation_time = other.m_animation_time;
    m_animation_speed = other.m_animation_speed;

    KW_ASSERT(
        other.m_animation_player == nullptr,
        "Copying animated geometry primitives with an animation player assigned is not allowed."
    );

    return *this;
}

AnimatedGeometryPrimitive& AnimatedGeometryPrimitive::operator=(AnimatedGeometryPrimitive&& other) {
    GeometryPrimitive::operator=(std::move(other));

    if (m_animation_player != nullptr) {
        m_animation_player->remove(*this);
    }

    m_skeleton_pose = std::move(other.m_skeleton_pose);
    m_animation = std::move(other.m_animation);
    m_animation_time = other.m_animation_time;
    m_animation_speed = other.m_animation_speed;

    KW_ASSERT(
        other.m_animation_player == nullptr,
        "Copying animated geometry primitives with an animation player assigned is not allowed."
    );

    return *this;
}

AnimationPlayer* AnimatedGeometryPrimitive::get_animation_player() const {
    return m_animation_player;
}

const SkeletonPose& AnimatedGeometryPrimitive::get_skeleton_pose() const {
    return m_skeleton_pose;
}

SkeletonPose& AnimatedGeometryPrimitive::get_skeleton_pose() {
    m_counter = ++acceleration_structure_counter;

    return m_skeleton_pose;
}

const SharedPtr<Animation>& AnimatedGeometryPrimitive::get_animation() const {
    return m_animation;
}

void AnimatedGeometryPrimitive::set_animation(SharedPtr<Animation> animation) {
    m_animation = std::move(animation);
}

Vector<float4x4> AnimatedGeometryPrimitive::get_model_space_joint_matrices(MemoryResource& memory_resource) {
    return Vector<float4x4>(m_skeleton_pose.get_model_space_matrices(), memory_resource);
}

float AnimatedGeometryPrimitive::get_animation_time() const {
    return m_animation_time;
}

void AnimatedGeometryPrimitive::set_animation_time(float value) {
    m_animation_time = value;
}

float AnimatedGeometryPrimitive::get_animation_speed() const {
    return m_animation_speed;
}

void AnimatedGeometryPrimitive::set_animation_speed(float value) {
    m_animation_speed = value;
}

UniquePtr<Primitive> AnimatedGeometryPrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<AnimatedGeometryPrimitive>(memory_resource, *this));
}

void AnimatedGeometryPrimitive::geometry_loaded() {
    KW_ASSERT(get_geometry() && get_geometry()->is_loaded(), "Geometry must be loaded.");

    const Skeleton* skeleton = get_geometry()->get_skeleton();
    if (skeleton != nullptr) {
        for (uint32_t i = 0; i < skeleton->get_joint_count(); i++) {
            m_skeleton_pose.set_joint_space_matrix(i, skeleton->get_bind_matrix(i));
        }
        m_skeleton_pose.build_model_space_matrices(*skeleton);
    }

    GeometryPrimitive::geometry_loaded();
}

} // namespace kw
