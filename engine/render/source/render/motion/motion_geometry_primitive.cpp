#include "render/motion/motion_geometry_primitive.h"
#include "render/blend_tree/blend_tree.h"
#include "render/blend_tree/nodes/blend_tree_node.h"
#include "render/geometry/geometry.h"
#include "render/geometry/geometry_manager.h"
#include "render/geometry/skeleton.h"
#include "render/material/material_manager.h"
#include "render/motion/motion_graph.h"
#include "render/motion/motion_graph_manager.h"
#include "render/scene/render_primitive_reflection.h"

#include <core/debug/assert.h>
#include <core/debug/log.h>
#include <core/io/markdown.h>
#include <core/io/markdown_utils.h>

namespace kw {

UniquePtr<Primitive> MotionGeometryPrimitive::create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node) {
    RenderPrimitiveReflection& render_reflection = dynamic_cast<RenderPrimitiveReflection&>(reflection);

    StringNode& motion_graph_node = node["motion_graph"].as<StringNode>();
    StringNode& geometry_node = node["geometry"].as<StringNode>();
    StringNode& material_node = node["material"].as<StringNode>();
    StringNode& shadow_material_node = node["shadow_material"].as<StringNode>();

    SharedPtr<MotionGraph> motion_graph = render_reflection.motion_graph_manager.load(motion_graph_node.get_value().c_str());
    SharedPtr<Geometry> geometry = render_reflection.geometry_manager.load(geometry_node.get_value().c_str());
    SharedPtr<Material> material = render_reflection.material_manager.load(material_node.get_value().c_str());
    SharedPtr<Material> shadow_material = render_reflection.material_manager.load(shadow_material_node.get_value().c_str());
    transform local_transform = MarkdownUtils::transform_from_markdown(node["local_transform"]);

    return static_pointer_cast<Primitive>(allocate_unique<MotionGeometryPrimitive>(
        reflection.memory_resource, reflection.memory_resource, motion_graph, geometry, material, shadow_material, local_transform
    ));
}

MotionGeometryPrimitive::MotionGeometryPrimitive(MemoryResource& memory_resource, SharedPtr<MotionGraph> motion_graph,
                                                 SharedPtr<Geometry> geometry, SharedPtr<Material> material,
                                                 SharedPtr<Material> shadow_material, const transform& local_transform)
    : AnimatedGeometryPrimitive(memory_resource, geometry, material, shadow_material, local_transform)
    , m_motion_graph(std::move(motion_graph))
    , m_attributes(memory_resource)
    , m_joints_model_pre_ik(memory_resource)
    , m_ik_targets(memory_resource)
    , m_previous_skeleton_pose(memory_resource)
    , m_motion_index(UINT32_MAX)
    , m_motion_time(0.f)
    , m_transition_time(0.f)
    , m_transition_duration(0.f)
{
}

MotionGeometryPrimitive::MotionGeometryPrimitive(const MotionGeometryPrimitive& other) = default;
MotionGeometryPrimitive::MotionGeometryPrimitive(MotionGeometryPrimitive&& other) noexcept = default;
MotionGeometryPrimitive::~MotionGeometryPrimitive() = default;
MotionGeometryPrimitive& MotionGeometryPrimitive::operator=(const MotionGeometryPrimitive& other) = default;
MotionGeometryPrimitive& MotionGeometryPrimitive::operator=(MotionGeometryPrimitive&& other) noexcept = default;

const SharedPtr<MotionGraph>& MotionGeometryPrimitive::get_motion_graph() const {
    return m_motion_graph;
}

void MotionGeometryPrimitive::set_motion_graph(SharedPtr<MotionGraph> motion_graph) {
    if (m_motion_graph != motion_graph) {
        m_motion_graph = std::move(motion_graph);
        m_motion_index = UINT32_MAX;
        m_motion_time = 0.f;
    }
}

void MotionGeometryPrimitive::emit_event(const String& name) {
    if (m_motion_graph && m_motion_graph->is_loaded()) {
        if (m_motion_index == UINT32_MAX) {
            m_motion_index = m_motion_graph->get_default_motion_index();
        }

        const Vector<MotionGraph::Motion>& motions = m_motion_graph->get_motions();
        KW_ASSERT(m_motion_index < motions.size(), "Invalid motion index.");

        const Vector<MotionGraph::Transition>& transitions = m_motion_graph->get_transitions();

        for (uint32_t transition_index : motions[m_motion_index].transitions) {
            KW_ASSERT(transition_index < transitions.size(), "Invalid transition index");

            if (transitions[transition_index].trigger_event == name) {
                KW_ASSERT(transitions[transition_index].destination < motions.size(), "Invalid motion index.");

                if (m_motion_time + transitions[transition_index].duration >= motions[m_motion_index].duration) {
                    m_motion_index = transitions[transition_index].destination;
                    m_motion_time = 0.f;

                    std::swap(m_previous_skeleton_pose, get_skeleton_pose());

                    m_transition_time = 0.f;
                    m_transition_duration = transitions[transition_index].duration;
                }

                break;
            }
        }
    }
}

uint32_t MotionGeometryPrimitive::get_motion_index() const {
    return m_motion_index;
}

float MotionGeometryPrimitive::get_motion_time() const {
    return m_motion_time;
}

void MotionGeometryPrimitive::frozen_fade(float duration) {
    std::swap(m_previous_skeleton_pose, get_skeleton_pose());

    m_transition_time = 0.f;
    m_transition_duration = duration;
}

float MotionGeometryPrimitive::get_attribute(const String& name) const {
    auto it = m_attributes.find(name);
    if (it != m_attributes.end()) {
        return it->second;
    }
    return 0.f;
}

void MotionGeometryPrimitive::set_attribute(const String& name, float value) {
    auto it = m_attributes.find(name);
    if (it != m_attributes.end()) {
        it->second = value;
    } else {
        m_attributes.emplace(String(name, *m_attributes.get_allocator().memory_resource), value);
    }
}

const Vector<float4x4>& MotionGeometryPrimitive::get_model_space_joint_pre_ik_matrices() const {
    return m_joints_model_pre_ik;
}

const float4& MotionGeometryPrimitive::get_ik_target(uint32_t joint_a, uint32_t joint_b, uint32_t joint_c) const {
    for (const IkTarget& ik_target : m_ik_targets) {
        if (ik_target.joint_a == joint_a && ik_target.joint_b == joint_b && ik_target.joint_c == joint_c) {
            return ik_target.target;
        }
    }

    static const float4 ZERO;
    return ZERO;
}

void MotionGeometryPrimitive::set_ik_target(uint32_t joint_a, uint32_t joint_b, uint32_t joint_c, const float4& target) {
    const SharedPtr<Geometry>& geometry = get_geometry();
    KW_ASSERT(geometry && geometry->is_loaded(), "Geometry must be loaded to set IK target.");

    const Skeleton* skeleton = geometry->get_skeleton();
    KW_ASSERT(skeleton != nullptr, "Geometry must have sekelton to set IK target.");

    uint32_t joint_count = skeleton->get_joint_count();
    KW_ASSERT(joint_a < joint_count && joint_b < joint_count && joint_c < joint_count, "Invalid IK joints.");

    for (auto it = m_ik_targets.begin(); it != m_ik_targets.end(); ++it) {
        if (it->joint_a == joint_a && it->joint_b == joint_b && it->joint_c == joint_c) {
            if (target.w != 0.f) {
                it->target = target;
            } else {
                m_ik_targets.erase(it);
            }
            return;
        }
    }

    if (target.w != 0.f) {
        m_ik_targets.push_back(IkTarget{ joint_a, joint_b, joint_c, target });
    }
}

UniquePtr<Primitive> MotionGeometryPrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<MotionGeometryPrimitive>(memory_resource, *this));
}

void MotionGeometryPrimitive::update_animation(MemoryResource& transient_memory_resource, float elapsed_time) {
    const SharedPtr<Geometry>& geometry = get_geometry();

    if (geometry && geometry->is_loaded() && m_motion_graph && m_motion_graph->is_loaded()) {
        if (m_motion_index == UINT32_MAX) {
            m_motion_index = m_motion_graph->get_default_motion_index();
        }

        const Vector<MotionGraph::Motion>& motions = m_motion_graph->get_motions();
        KW_ASSERT(m_motion_index < motions.size(), "Invalid motion index.");

        SkeletonPose& skeleton_pose = get_skeleton_pose();

        BlendTreeContext context{
            m_attributes,
            transient_memory_resource,
            m_motion_time
        };

        // Unlike assignment operator, lerp won't use allocators from other skeleton pose.
        skeleton_pose.lerp(motions[m_motion_index].blend_tree->get_root_node()->compute(context), 1.f);

        if (m_transition_duration > 0.f) {
            skeleton_pose.lerp(m_previous_skeleton_pose, 1.f - m_transition_time / m_transition_duration);

            m_transition_time += elapsed_time;

            if (m_transition_time >= m_transition_duration) {
                m_transition_time = 0.f;
                m_transition_duration = 0.f;
            }
        }

        m_motion_time += elapsed_time;

        const Skeleton* skeleton = geometry->get_skeleton();
        KW_ASSERT(skeleton != nullptr, "Geometry must have skeleton for motion geometry primitive.");

        skeleton_pose.build_model_space_matrices(*skeleton);

        const Vector<transform>& joint_space_transforms = skeleton_pose.get_joint_space_transforms();
        const Vector<float4x4>& model_space_matrices = skeleton_pose.get_model_space_matrices();

        m_joints_model_pre_ik.assign(model_space_matrices.begin(), model_space_matrices.end());

        // Simple Two Joint IK
        // https://theorangeduck.com/page/simple-two-joint
        for (const IkTarget& ik_target : m_ik_targets) {
            KW_ASSERT(
                ik_target.joint_a < model_space_matrices.size() &&
                ik_target.joint_b < model_space_matrices.size() &&
                ik_target.joint_c < model_space_matrices.size(),
                "Unexpected model space matrices."
            );

            transform joint_a_model = transform(model_space_matrices[ik_target.joint_a]);
            transform joint_b_model = transform(model_space_matrices[ik_target.joint_b]);
            transform joint_c_model = transform(model_space_matrices[ik_target.joint_c]);

            const float3& a = joint_a_model.translation;
            const float3& b = joint_b_model.translation;
            const float3& c = joint_c_model.translation;

            float3 t = ik_target.target.xyz * inverse(get_global_transform());

            const quaternion& a_gr = joint_a_model.rotation;
            const quaternion& b_gr = joint_b_model.rotation;

            transform joint_a_joint = joint_space_transforms[ik_target.joint_a];
            transform joint_b_joint = joint_space_transforms[ik_target.joint_b];

            quaternion& a_lr = joint_a_joint.rotation;
            quaternion& b_lr = joint_b_joint.rotation;

            float lab = length(b - a);
            float lcb = length(b - c);
            float lat = clamp(length(t - a), EPSILON, lab + lcb - EPSILON);

            float ac_ab_0 = acos(clamp(dot(normalize(c - a), normalize(b - a)), -1.f, 1.f));
            float ba_bc_0 = acos(clamp(dot(normalize(a - b), normalize(c - b)), -1.f, 1.f));
            float ac_at_0 = acos(clamp(dot(normalize(c - a), normalize(t - a)), -1.f, 1.f));
            
            float ac_ab_1 = acos(clamp((sqr(lcb) - sqr(lab) - sqr(lat)) / (-2.f * lab * lat), -1.f, 1.f));
            float ba_bc_1 = acos(clamp((sqr(lat) - sqr(lab) - sqr(lcb)) / (-2.f * lab * lcb), -1.f, 1.f));

            float3 axis0 = normalize(cross(c - a, b - a));
            float3 axis1 = normalize(cross(c - a, t - a));

            quaternion r0 = quaternion::rotation(axis0 * inverse(a_gr), ac_ab_1 - ac_ab_0);
            quaternion r1 = quaternion::rotation(axis0 * inverse(b_gr), ba_bc_1 - ba_bc_0);
            quaternion r2 = quaternion::rotation(axis1 * inverse(a_gr), ac_at_0);

            a_lr = slerp(a_lr, a_lr * r0 * r2, ik_target.target.w);
            b_lr = slerp(b_lr, b_lr * r1, ik_target.target.w);

            skeleton_pose.set_joint_space_transform(ik_target.joint_a, joint_a_joint);
            skeleton_pose.set_joint_space_transform(ik_target.joint_b, joint_b_joint);
        }

        skeleton_pose.build_model_space_matrices(*skeleton);
        skeleton_pose.apply_inverse_bind_matrices(*skeleton);
    }
}

} // namespace kw
