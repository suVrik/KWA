#include "render/acceleration_structure/octree_acceleration_structure.h"
#include "render/acceleration_structure/acceleration_structure_primitive.h"

#include <core/debug/assert.h>
#include <core/scene/primitive.h>

namespace kw {

constexpr uint32_t OCTREE_POSITIVE_X = 0;
constexpr uint32_t OCTREE_NEGATIVE_X = 1 << 0;
constexpr uint32_t OCTREE_POSITIVE_Y = 0;
constexpr uint32_t OCTREE_NEGATIVE_Y = 1 << 1;
constexpr uint32_t OCTREE_POSITIVE_Z = 0;
constexpr uint32_t OCTREE_NEGATIVE_Z = 1 << 2;

static float3 OCTREE_EXTENT_FACTORS[] = {
    float3( 1.f,  1.f,  1.f),
    float3(-1.f,  1.f,  1.f),
    float3( 1.f, -1.f,  1.f),
    float3(-1.f, -1.f,  1.f),
    float3( 1.f,  1.f, -1.f),
    float3(-1.f,  1.f, -1.f),
    float3( 1.f, -1.f, -1.f),
    float3(-1.f, -1.f, -1.f),
};


OctreeNode::OctreeNode(MemoryResource& persistent_memory_resource)
    : primitives(persistent_memory_resource)
{
}

OctreeAccelerationStructure::OctreeAccelerationStructure(MemoryResource& persistent_memory_resource, const float3& center, const float3& extent, uint32_t max_depth)
    : OctreeNode(persistent_memory_resource)
    , m_memory_resource(persistent_memory_resource)
    , m_max_depth(max_depth)
    , m_count(0)
{
    KW_ASSERT(extent.x > 0.f, "Invalid octree extent.");
    KW_ASSERT(extent.y > 0.f, "Invalid octree extent.");
    KW_ASSERT(extent.z > 0.f, "Invalid octree extent.");

    bounds.center = center;
    bounds.extent = extent;
}

void OctreeAccelerationStructure::add(AccelerationStructurePrimitive& primitive) {
    std::lock_guard lock_guard(m_shared_mutex);

    KW_ASSERT(primitive.m_acceleration_structure == nullptr, "Primitive is already is in this acceleration structure.");
    primitive.m_acceleration_structure = this;

    OctreeNode& node = find_node(primitive.get_bounds(), *this);
    KW_ASSERT(std::find(node.primitives.begin(), node.primitives.end(), &primitive) == node.primitives.end());

    node.primitives.push_back(&primitive);

    primitive.m_node = &node;

    m_count++;
}

void OctreeAccelerationStructure::remove(AccelerationStructurePrimitive& primitive) {
    std::lock_guard lock_guard(m_shared_mutex);

    KW_ASSERT(primitive.m_acceleration_structure == this, "Primitive is not in this acceleration structure.");
    primitive.m_acceleration_structure = nullptr;

    OctreeNode* node = static_cast<OctreeNode*>(primitive.m_node);
    KW_ASSERT(node != nullptr);

    auto it = std::find(node->primitives.begin(), node->primitives.end(), &primitive);
    KW_ASSERT(it != node->primitives.end());

    *it = node->primitives.back();
    node->primitives.pop_back();

    primitive.m_node = nullptr;

    KW_ASSERT(m_count > 0);
    m_count--;
}

void OctreeAccelerationStructure::update(AccelerationStructurePrimitive& primitive) {
    std::lock_guard lock_guard(m_shared_mutex);

    KW_ASSERT(primitive.m_acceleration_structure == this, "Primitive is not in this acceleration structure.");

    OctreeNode* node = static_cast<OctreeNode*>(primitive.m_node);
    KW_ASSERT(node != nullptr);

    const aabbox& bounds = primitive.get_bounds();

    if (bounds.center.x - bounds.extent.x <  node->bounds.center.x - node->bounds.extent.x ||
        bounds.center.y - bounds.extent.y <  node->bounds.center.y - node->bounds.extent.y ||
        bounds.center.z - bounds.extent.z <  node->bounds.center.z - node->bounds.extent.z ||
        bounds.center.x + bounds.extent.x >= node->bounds.center.x + node->bounds.extent.x ||
        bounds.center.y + bounds.extent.y >= node->bounds.center.y + node->bounds.extent.y ||
        bounds.center.z + bounds.extent.z >= node->bounds.center.z + node->bounds.extent.z)
    {
        auto it = std::find(node->primitives.begin(), node->primitives.end(), &primitive);
        KW_ASSERT(it != node->primitives.end());

        *it = node->primitives.back();
        node->primitives.pop_back();

        OctreeNode& node = find_node(bounds, *this);
        node.primitives.push_back(&primitive);

        primitive.m_node = &node;
    }
}

template <typename Bounds>
void OctreeAccelerationStructure::collect_primitives(const OctreeNode& node, const Bounds& bounds, Vector<AccelerationStructurePrimitive*>& output) const {
    for (AccelerationStructurePrimitive* primitive : node.primitives) {
        if (intersect(primitive->get_bounds(), bounds)) {
            output.push_back(primitive);
        }
    }

    for (const UniquePtr<OctreeNode>& child : node.children) {
        if (child && intersect(child->bounds, bounds)) {
            collect_primitives(*child, bounds, output);
        }
    }
}

Vector<AccelerationStructurePrimitive*> OctreeAccelerationStructure::query(MemoryResource& memory_resource, const aabbox& bounds) const {
    std::shared_lock shared_lock(m_shared_mutex);

    Vector<AccelerationStructurePrimitive*> result(memory_resource);
    result.reserve(64);

    collect_primitives(*this, bounds, result);
    
    return result;
}

Vector<AccelerationStructurePrimitive*> OctreeAccelerationStructure::query(MemoryResource& memory_resource, const frustum& frustum) const {
    std::shared_lock shared_lock(m_shared_mutex);

    Vector<AccelerationStructurePrimitive*> result(memory_resource);
    result.reserve(64);
    
    collect_primitives(*this, frustum, result);
    
    return result;
}

size_t OctreeAccelerationStructure::get_count() const {
    std::shared_lock shared_lock(m_shared_mutex);

    return m_count;
}

OctreeNode& OctreeAccelerationStructure::find_node(const aabbox& bounds, OctreeNode& node, uint32_t depth) {
    if (depth >= m_max_depth) {
        return node;
    }

    uint32_t index = 0;

    if (bounds.center.x - bounds.extent.x >= node.bounds.center.x) {
        index |= OCTREE_POSITIVE_X;
    } else if (bounds.center.x + bounds.extent.x < node.bounds.center.x) {
        index |= OCTREE_NEGATIVE_X;
    } else {
        return node;
    }

    if (bounds.center.y - bounds.extent.y >= node.bounds.center.y) {
        index |= OCTREE_POSITIVE_Y;
    } else if (bounds.center.y + bounds.extent.y < node.bounds.center.y) {
        index |= OCTREE_NEGATIVE_Y;
    } else {
        return node;
    }

    if (bounds.center.z - bounds.extent.z >= node.bounds.center.z) {
        index |= OCTREE_POSITIVE_Z;
    } else if (bounds.center.z + bounds.extent.z < node.bounds.center.z) {
        index |= OCTREE_NEGATIVE_Z;
    } else {
        return node;
    }

    UniquePtr<OctreeNode>& child = node.children[index];
    if (!child) {
        float extent_x = node.bounds.extent.x / 2.f;
        float extent_y = node.bounds.extent.y / 2.f;
        float extent_z = node.bounds.extent.z / 2.f;

        float center_x = node.bounds.center.x + OCTREE_EXTENT_FACTORS[index].x * extent_x;
        float center_y = node.bounds.center.y + OCTREE_EXTENT_FACTORS[index].y * extent_y;
        float center_z = node.bounds.center.z + OCTREE_EXTENT_FACTORS[index].z * extent_z;

        child = allocate_unique<OctreeNode>(m_memory_resource, m_memory_resource);
        child->bounds = aabbox{
            float3{ center_x, center_y, center_z },
            float3{ extent_x, extent_y, extent_z }
        };
    }

    return find_node(bounds, *child, depth + 1);
}

} // namespace kw
