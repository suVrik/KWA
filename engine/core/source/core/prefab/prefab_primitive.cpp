#include "core/prefab/prefab_primitive.h"
#include "core/debug/assert.h"
#include "core/io/markdown.h"
#include "core/io/markdown_utils.h"
#include "core/prefab/prefab_manager.h"
#include "core/prefab/prefab_prototype.h"
#include "core/scene/primitive_reflection.h"

namespace kw {

UniquePtr<Primitive> PrefabPrimitive::create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node) {
    StringNode& prefab_prototype_node = node["prefab_prototype"].as<StringNode>();

    SharedPtr<PrefabPrototype> prefab_prototype = prefab_prototype_node.get_value().empty() ? nullptr : reflection.prefab_manager.load(prefab_prototype_node.get_value().c_str());
    transform local_transform = MarkdownUtils::transform_from_markdown(node["local_transform"]);

    return static_pointer_cast<Primitive>(allocate_unique<PrefabPrimitive>(
        reflection.memory_resource, reflection.memory_resource, prefab_prototype, local_transform
    ));
}

PrefabPrimitive::PrefabPrimitive(MemoryResource& persistent_memory_resource,
                                 SharedPtr<PrefabPrototype> prefab_prototype,
                                 const transform& local_transform)
    : Primitive(local_transform)
    , m_children(persistent_memory_resource)
    , m_prefab_prototype(std::move(prefab_prototype))
{
    if (m_prefab_prototype) {
        // If prefab primitive is already loaded, `prefab_prototype_loaded` will be called immediately.
        m_prefab_prototype->subscribe(*this);
    }
}

PrefabPrimitive::PrefabPrimitive(const PrefabPrimitive& other)
    : Primitive(other)
    , m_children(*other.m_children.get_allocator().memory_resource)
    , m_prefab_prototype(other.m_prefab_prototype)
{
    KW_ASSERT(
        other.m_children.empty(),
        "Copying non-empty prefabs is not allowed."
    );

    if (m_prefab_prototype) {
        // If prefab primitive is already loaded, `prefab_prototype_loaded` will be called immediately.
        m_prefab_prototype->subscribe(*this);
    }
}

PrefabPrimitive::~PrefabPrimitive() {
    // Primitive removes itself from prefab in destructor.
    while (!m_children.empty()) {
        m_children.front().reset();
    }

    if (m_prefab_prototype) {
        // No effect if `prefab_prototype_loaded` for this primitive was already called.
        m_prefab_prototype->unsubscribe(*this);
    }
}

PrefabPrimitive& PrefabPrimitive::operator=(const PrefabPrimitive& other) {
    Primitive::operator=(other);

    KW_ASSERT(
        other.m_children.empty(),
        "Copying non-empty prefabs is not allowed."
    );

    if (m_prefab_prototype) {
        // No effect if `prefab_prototype_loaded` for this primitive was already called.
        m_prefab_prototype->unsubscribe(*this);
    }

    m_prefab_prototype = other.m_prefab_prototype;

    if (m_prefab_prototype) {
        // If prefab primitive is already loaded, `prefab_prototype_loaded` will be called immediately.
        m_prefab_prototype->subscribe(*this);
    }

    return *this;
}

const SharedPtr<PrefabPrototype>& PrefabPrimitive::get_prefab_prototype() const {
    return m_prefab_prototype;
}

void PrefabPrimitive::set_prefab_prototype(SharedPtr<PrefabPrototype> prefab_prototype) {
    if (m_prefab_prototype) {
        // No effect if `prefab_prototype_loaded` for this primitive was already called.
        m_prefab_prototype->unsubscribe(*this);
    }

    m_prefab_prototype = std::move(prefab_prototype);

    if (m_prefab_prototype) {
        // If prefab primitive is already loaded, `prefab_prototype_loaded` will be called immediately.
        m_prefab_prototype->subscribe(*this);
    }
}

void PrefabPrimitive::add_child(UniquePtr<Primitive>&& primitive) {
    KW_ASSERT(primitive->m_parent == nullptr, "Primitive already has a parent.");

    Primitive& primitive_ = *primitive;

    m_children.push_back(std::move(primitive));

    primitive_.m_parent = this;

    // Primitive joins this prefab's system coordinate.
    primitive_.m_global_transform = primitive_.m_local_transform * m_global_transform;

    // Update primitive's global transform and bounds recursively.
    primitive_.global_transform_updated();

    // Notify all parents about added child after it's added.
    child_added(primitive_);
}

void PrefabPrimitive::add_children(Vector<UniquePtr<Primitive>>& children) {
    m_children.reserve(m_children.size() + children.size());

    for (UniquePtr<Primitive>& primitive : children) {
        KW_ASSERT(primitive != nullptr, "Invalid primitive.");

        add_child(std::move(primitive));
    }
}

UniquePtr<Primitive> PrefabPrimitive::remove_child(Primitive& primitive) {
    KW_ASSERT(primitive.m_parent == this, "Invalid primitive.");

    auto it = std::find_if(m_children.begin(), m_children.end(), [&](UniquePtr<Primitive>& child) {
        return child.get() == &primitive;
    });
    KW_ASSERT(it != m_children.end());

    // Notify all parents about removed child before it's removed.
    child_removed(primitive);

    UniquePtr<Primitive> result = std::move(*it);
    m_children.erase(it);

    primitive.m_parent = nullptr;

    // Primitive leaves this prefab's system coordinate.
    primitive.m_global_transform = primitive.m_local_transform;

    // Update primitive's global transform and bounds recursively.
    primitive.global_transform_updated();

    return result;
}

const Vector<UniquePtr<Primitive>>& PrefabPrimitive::get_children() const {
    return m_children;
}

UniquePtr<Primitive> PrefabPrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<PrefabPrimitive>(memory_resource, *this));
}

void PrefabPrimitive::global_transform_updated() {
    // Global transform had been updated outside.

    for (UniquePtr<Primitive>& child : m_children) {
        // Update child's global transform too.
        child->m_global_transform = child->m_local_transform * m_global_transform;

        // Render primitives must update their bounds, prefab primitives must propagate global transform.
        child->global_transform_updated();
    }
}

void PrefabPrimitive::prefab_prototype_loaded() {
    KW_ASSERT(
        m_prefab_prototype != nullptr && m_prefab_prototype->is_loaded(),
        "Prefab prototype is exprected to be loaded."
    );

    // Primitive removes itself from prefab in destructor.
    while (!m_children.empty()) {
        m_children.front().reset();
    }

    const Vector<UniquePtr<Primitive>>& primitives = m_prefab_prototype->get_primitives();

    m_children.reserve(primitives.size());

    for (const UniquePtr<Primitive>& primitive : primitives) {
        add_child(primitive->clone(*m_children.get_allocator().memory_resource));
    }
}

void PrefabPrimitive::child_added(Primitive& primitive) {
    if (m_parent != nullptr) {
        m_parent->child_added(primitive);
    }
}

void PrefabPrimitive::child_removed(Primitive& primitive) {
    if (m_parent != nullptr) {
        m_parent->child_removed(primitive);
    }
}

} // namespace kw
