#include "render/container/container_primitive.h"
#include "render/container/container_manager.h"
#include "render/container/container_prototype.h"
#include "render/scene/primitive_reflection.h"

#include <core/debug/assert.h>
#include <core/io/markdown.h>
#include <core/io/markdown_utils.h>

namespace kw {

UniquePtr<Primitive> ContainerPrimitive::create_from_markdown(const PrimitiveReflectionDescriptor& primitive_reflection_descriptor) {
    KW_ASSERT(primitive_reflection_descriptor.primitive_node != nullptr);
    KW_ASSERT(primitive_reflection_descriptor.container_manager != nullptr);
    KW_ASSERT(primitive_reflection_descriptor.persistent_memory_resource != nullptr);

    ObjectNode& node = *primitive_reflection_descriptor.primitive_node;
    StringNode& container_prototype_node = node["container_prototype"].as<StringNode>();

    MemoryResource& memory_resource = *primitive_reflection_descriptor.persistent_memory_resource;
    SharedPtr<ContainerPrototype> container_prototype = container_prototype_node.get_value().empty() ? nullptr : primitive_reflection_descriptor.container_manager->load(container_prototype_node.get_value().c_str());
    transform local_transform = MarkdownUtils::transform_from_markdown(node["local_transform"]);

    return static_pointer_cast<Primitive>(allocate_unique<ContainerPrimitive>(
        memory_resource, memory_resource, container_prototype, local_transform
    ));
}

ContainerPrimitive::ContainerPrimitive(MemoryResource& persistent_memory_resource,
                                       SharedPtr<ContainerPrototype> container_prototype,
                                       const transform& local_transform)
    : Primitive(local_transform)
    , m_children(persistent_memory_resource)
    , m_container_prototype(std::move(container_prototype))
{
    if (m_container_prototype) {
        // If container primitive is already loaded, `container_prototype_loaded` will be called immediately.
        m_container_prototype->subscribe(*this);
    }
}

ContainerPrimitive::ContainerPrimitive(const ContainerPrimitive& other)
    : Primitive(other)
    , m_children(*other.m_children.get_allocator().memory_resource)
    , m_container_prototype(other.m_container_prototype)
{
    KW_ASSERT(
        other.m_children.empty(),
        "Copying non-empty containers is not allowed."
    );

    if (m_container_prototype) {
        // If container primitive is already loaded, `container_prototype_loaded` will be called immediately.
        m_container_prototype->subscribe(*this);
    }
}

ContainerPrimitive::~ContainerPrimitive() {
    // Primitive removes itself from container in destructor.
    while (!m_children.empty()) {
        m_children.front().reset();
    }

    if (m_container_prototype) {
        // No effect if `container_prototype_loaded` for this primitive was already called.
        m_container_prototype->unsubscribe(*this);
    }
}

ContainerPrimitive& ContainerPrimitive::operator=(const ContainerPrimitive& other) {
    Primitive::operator=(other);

    KW_ASSERT(
        other.m_children.empty(),
        "Copying non-empty containers is not allowed."
    );

    if (m_container_prototype) {
        // No effect if `container_prototype_loaded` for this primitive was already called.
        m_container_prototype->unsubscribe(*this);
    }

    m_container_prototype = other.m_container_prototype;

    if (m_container_prototype) {
        // If container primitive is already loaded, `container_prototype_loaded` will be called immediately.
        m_container_prototype->subscribe(*this);
    }

    return *this;
}

const SharedPtr<ContainerPrototype>& ContainerPrimitive::get_container_prototype() const {
    return m_container_prototype;
}

void ContainerPrimitive::set_container_prototype(SharedPtr<ContainerPrototype> container_prototype) {
    if (m_container_prototype) {
        // No effect if `container_prototype_loaded` for this primitive was already called.
        m_container_prototype->unsubscribe(*this);
    }

    m_container_prototype = std::move(container_prototype);

    if (m_container_prototype) {
        // If container primitive is already loaded, `container_prototype_loaded` will be called immediately.
        m_container_prototype->subscribe(*this);
    }
}

void ContainerPrimitive::add_child(UniquePtr<Primitive>&& primitive) {
    KW_ASSERT(primitive->m_parent == nullptr, "Primitive already has a parent.");

    Primitive& primitive_ = *primitive;

    m_children.push_back(std::move(primitive));

    primitive_.m_parent = this;

    // Primitive joins this container's system coordinate.
    primitive_.m_global_transform = primitive_.m_local_transform * m_global_transform;

    // Update primitive's global transform and bounds recursively.
    primitive_.global_transform_updated();

    // Notify all parents about added child after it's added.
    child_added(primitive_);
}

void ContainerPrimitive::add_children(Vector<UniquePtr<Primitive>>& children) {
    m_children.reserve(m_children.size() + children.size());

    for (UniquePtr<Primitive>& primitive : children) {
        KW_ASSERT(primitive != nullptr, "Invalid primitive.");

        add_child(std::move(primitive));
    }
}

UniquePtr<Primitive> ContainerPrimitive::remove_child(Primitive& primitive) {
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

    // Primitive leaves this container's system coordinate.
    primitive.m_global_transform = primitive.m_local_transform;

    // Update primitive's global transform and bounds recursively.
    primitive.global_transform_updated();

    return result;
}

const Vector<UniquePtr<Primitive>>& ContainerPrimitive::get_children() const {
    return m_children;
}

UniquePtr<Primitive> ContainerPrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<ContainerPrimitive>(memory_resource, *this));
}

void ContainerPrimitive::global_transform_updated() {
    // Global transform had been updated outside.

    for (UniquePtr<Primitive>& child : m_children) {
        // Update child's global transform too.
        child->m_global_transform = child->m_local_transform * m_global_transform;

        // Render primitives must update their bounds, container primitives must propagate global transform.
        child->global_transform_updated();
    }
}

void ContainerPrimitive::container_prototype_loaded() {
    KW_ASSERT(
        m_container_prototype != nullptr && m_container_prototype->is_loaded(),
        "Container prototype is exprected to be loaded."
    );

    // Primitive removes itself from container in destructor.
    while (!m_children.empty()) {
        m_children.front().reset();
    }

    const Vector<UniquePtr<Primitive>>& primitives = m_container_prototype->get_primitives();

    m_children.reserve(primitives.size());

    for (const UniquePtr<Primitive>& primitive : primitives) {
        add_child(primitive->clone(*m_children.get_allocator().memory_resource));
    }
}

void ContainerPrimitive::child_added(Primitive& primitive) {
    if (m_parent != nullptr) {
        m_parent->child_added(primitive);
    }
}

void ContainerPrimitive::child_removed(Primitive& primitive) {
    if (m_parent != nullptr) {
        m_parent->child_removed(primitive);
    }
}

} // namespace kw
